#include "defs.h"
#include <iostream>

typedef unsigned int Index;
typedef unsigned int Key;
typedef int Level;

class LoserTree {
  Level const height;
  struct Node {
    Index index;
    Key key;
    Node(){};
    Node(Index index, Key key) : index(index), key(key) {}
    // void swap(Node &candidate) {
    //     Node temp = this;
    //     this = candidate;
    //     candidate = temp;
    // }

    bool less(const Node &candidate) {
      if (key < candidate.key) {
        return true;
      }
      return false;
    }
  } *const heap;

public:
  LoserTree(Level const h) : height(h), heap(new Node[1 << h]) { TRACE(true); }
  ~LoserTree() { delete[] heap; }

  Index capacity() { return Index(1 << height); }
  Index root() { return Index(0); }
  void leaf(Index index, Index &slot) { slot = capacity() + index; }
  void parent(Index &slot) { slot /= 2; }
  void leaf(Index index, Index &slot, Level &level) {
    level = 0;
    leaf(index, slot);
  }
  void parent(Index &slot, Level &level) {
    ++level;
    parent(slot);
  }
  Key early_fence(Index index) { return Key(index); }
  Key late_fence(Index index) { return ~Key(index); }
  void pass(Index index, Key key) {
    Node candidate(index, key);
    Index slot;
    for (slot = capacity() + index; (slot /= 2) != 0;) {
      if (heap[slot].less(candidate)) {
        Node temp = heap[slot];
        heap[slot] = candidate;
        candidate = temp;
      }
    }
    heap[root()] = candidate;
  }

  bool empty() {
    Node &hr = heap[root()];
    while (hr.key == early_fence(hr.index))
      pass(hr.index, late_fence(hr.index));
    return hr.key == late_fence(hr.index);
  }

  Index poptop(bool invalidate) {
    if (empty())
      return -1;
    if (invalidate) {
      std::cout << "popped key - " << heap[root()].key << std::endl;
      heap[root()].key = early_fence(heap[root()].index);
    }

    return heap[root()].index;
  }
  Index top() { return poptop(false); }

  Index pop() { return poptop(true); }
  void push(Index index, Key key) {
    pass(index, early_fence(capacity()) + key);
  }
  void insert(Index index, Key key) { push(index, key); }
  void update(Index index, Key key) { push(index, key); }
  void deleteKey(Index index) { pass(index, late_fence(index)); }
};

int main() {
  std::cout << "Ranjitha" << std::endl;
  Level level(2);
  LoserTree ltre(level);
  for (int i = 0; i < 4; i++) {
    ltre.insert(i, 4 - i);
  }

  while (!ltre.empty()) {
    std::cout << ltre.pop() << std::endl;
  }
}
