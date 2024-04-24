#include "Record.h"
#include "defs.h"

typedef unsigned int RunId;
typedef unsigned int RecordId;
typedef int Level;

template <typename Compare> class LoserTree {
  Level const height;
  Index_r heap;
  Compare const cmp;
  uint32_t const cap;

public:
  LoserTree(Level const h, const Compare &cmp, Index_r &index)
      : height(h), heap(index), cmp(cmp), cap(capacity()) {
    TRACE(true);
    uint32_t cap = capacity();
    for (uint32_t i = 0; i < cap; i++) {
      // MergeInd temp = {cap + 1, early_fence(i)};
      heap[i].record_id = early_fence(i);
      heap[i].run_id = cap + 1;
    }
  }
  ~LoserTree() = default;

  bool less(MergeInd &a, MergeInd &b) {
    bool achanged = false;
    bool bchanged = false;
    if (a.record_id == late_fence(a.run_id) ||
        a.record_id == early_fence(a.run_id)) {
      return false;
    } else {
      achanged = true;
    }

    if (b.record_id == late_fence(b.run_id) ||
        b.record_id == early_fence(b.run_id)) {
      return true;
    } else {
      bchanged = true;
    }

    if (achanged) {
      a.record_id = a.record_id - capacity();
    }
    if (bchanged) {
      b.record_id = b.record_id - capacity();
    }

    bool compare = cmp(a, b);
    if (achanged) {
      a.record_id = a.record_id + capacity();
    }
    if (bchanged) {
      b.record_id = b.record_id + capacity();
    }
    return compare;
  }

  RunId capacity() const { return RunId(1 << height); }
  RunId root() const { return RunId(0); }
  RunId leaf(RunId index, RunId &slot) const {
    return slot = capacity() + index;
  }
  RunId parent(RunId &slot) const { return slot /= 2; }
  void leaf(RunId index, RunId &slot, Level &level) const {
    level = 0;
    leaf(index, slot);
  }
  void parent(RunId &slot, Level &level) const {
    ++level;
    parent(slot);
  }
  RecordId early_fence(RunId index) const { return RecordId(index); }
  RecordId late_fence(RunId index) const { return ~RecordId(index); }
  void pass(RunId index, RecordId key) {
    MergeInd candidate = {index, key};
    RunId slot;
    for (leaf(index, slot); parent(slot) != 0;) {
      if (heap[slot].run_id > capacity()) {
        heap[slot] = candidate;
        return;
      }
      if (less(heap[slot], candidate)) {
        MergeInd temp = heap[slot];
        heap[slot] = candidate;
        candidate = temp;
      }
    }
    heap[root()] = candidate;
  }

  bool empty() {
    MergeInd &hr = heap[root()];
    while (hr.record_id == early_fence(hr.run_id))
      pass(hr.run_id, late_fence(hr.run_id));
    return hr.record_id == late_fence(hr.run_id);
  }

  MergeInd poptop(bool invalidate) {
    if (invalidate) {
      MergeInd popped = heap[root()];
      popped.record_id = popped.record_id - early_fence(capacity());
      heap[root()].record_id = early_fence(heap[root()].run_id);
      return popped;
    }
    return heap[root()];
  }

  MergeInd top() { return poptop(false); }
  MergeInd pop() { return poptop(true); }

  void push(RunId index, RecordId key) {
    pass(index, early_fence(capacity()) + key);
  }
  void insert(RunId index, RecordId key) { push(index, key); }
  void update(RunId index, RecordId key) { push(index, key); }
  void deleteRecordId(RunId index) { pass(index, late_fence(index)); }
};
