#include "SortFunc.h"
#include "Record.h"
#include <algorithm>
#include <cstddef>

void apply_permut(RecordArr_t &records, Index_t &index,
                  RowCount const n_records) {
  for (uint16_t i = 0; i < n_records; i++) {
    uint16_t current = i;
    while (i != index[current]) {
      uint16_t next = index[current];
      swap(records[current], records[next]);
      index[current] = current;
      current = next;
    }
    index[current] = current;
  }
} // apply_permut (in-place)

void apply_permut(RecordArr_t const &records, RecordArr_t &out,
                  Index_t const &index, RowCount const n_records) {
  for (uint16_t i = 0; i < n_records; i++) {
    out[i] = records[index[i]];
  }
} // apply_permut (out-of-place)

void incache_sort(RecordArr_t &records, Index_t &index,
                  RowCount const n_records) {
  auto begin = index.begin();
  auto end = index.begin() + n_records;
  if (end > index.end()) {
    throw std::out_of_range("incache_sort: index out of range");
  } // if

  for (uint16_t i = 0; i < end - begin; ++i) {
    index[i] = i;
  } // for
  std::sort(begin, end, [&records](uint32_t const a, uint32_t const b) {
    return records[a] < records[b];
  });

  apply_permut(records, index, end - begin);
} // incache_sort (in-place)

void incache_sort(RecordArr_t const &records, RecordArr_t &out, Index_t &index,
                  RowCount const n_records) {
  auto begin = index.begin();
  auto end = index.begin() + n_records;
  if (end > index.end()) {
    throw std::out_of_range("incache_sort: index out of range");
  } // if

  for (uint16_t i = 0; i < end - begin; ++i) {
    index[i] = i;
  } // for
  std::sort(begin, end, [&records](uint32_t const a, uint32_t const b) {
    return records[a] < records[b];
  });

  apply_permut(records, out, index, end - begin);
} // incache_sort (out-of-place)

void inmem_merge(RecordArr_t const &records, RecordArr_t &out, Index_r &index,
                 RowCount const run_size, RowCount const n_runs) {
  for (uint32_t i = 0; i < n_runs; ++i) {
    index[i] = {i, 0};
  } // for
  auto begin = index.begin();
  auto end = index.begin() + n_runs;
  if (end > index.end()) {
    throw std::out_of_range("inmem_merge: index out of range");
  } // if

  auto cmp = [&records, &run_size](MergeInd const &a, MergeInd const &b) {
    return records[a.run_id * run_size + a.record_id] >
           records[b.run_id * run_size + b.record_id];
  };

  std::make_heap(begin, end, cmp);

  std::size_t out_ind = 0;
  while (begin != end) {
    out[out_ind++] = records[begin->run_id * run_size + begin->record_id];
    std::pop_heap(begin, end, cmp);

    if (++((end - 1)->record_id) >= run_size) {
      --end;
    } // if

    std::push_heap(begin, end, cmp);
  }
} // inmem_merge
