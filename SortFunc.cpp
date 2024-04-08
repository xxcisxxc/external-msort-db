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

void inmem_merge(RecordArr_t const &records, OutBuffer out, Device *hd,
                 Index_r &index, RunInfo run_info, Record_t *outputWitnessRecord) {
  RowCount const run_size = run_info.run_size;
  RowCount const n_runs = run_info.n_runs;
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
  bool first = true;
  while (begin != end) {
    out.out[out_ind++] = records[begin->run_id * run_size + begin->record_id];
    std::pop_heap(begin, end, cmp);
    if(out_ind == 1 && first) {
        Record_t::copy_rec(out.out[out_ind - 1], outputWitnessRecord);
        first = false;
    } else {
        (*outputWitnessRecord).x_or(out.out[out_ind - 1]);
    }
    if (++((end - 1)->record_id) >= run_size) {
      --end;
    } // if

    std::push_heap(begin, end, cmp);
    if (out_ind == out.out_size) {
      // TODO: check return value
      hd->eappend(reinterpret_cast<char *>(out.out.data()),
                  out.out_size * Record_t::bytes);
      out_ind = 0;
    } // if
  }
  if (out_ind > 0) {
    // TODO: check return value
    hd->eappend(reinterpret_cast<char *>(out.out.data()),
                out_ind * Record_t::bytes);
  } // if
} // inmem_merge

void external_merge(RecordArr_t &records, OutBuffer out, DeviceInOut dev,
                    Index_r &index, ExRunInfo run_info, Record_t *outputWitnessRecord) {
  RowCount const run_size = run_info.run_size;
  RowCount const n_runs = run_info.n_runs;

  for (uint32_t run_id = 0; run_id < n_runs; ++run_id) { // n_runs in sdd: 8
    dev.hd_in->eread(
        reinterpret_cast<char *>((records + run_size * run_id).data()),
        run_size * Record_t::bytes,
        (run_info.exrun_size * run_id) * Record_t::bytes);
  } // for

  for (uint32_t i = 0; i < n_runs; ++i) {
    index[i] = {i, 0};
  } // for
  auto begin = index.begin();
  auto end = index.begin() + n_runs;
  if (end > index.end()) {
    throw std::out_of_range("inmem_merge: index out of range");
  } // if

  auto cmp = [&records, &run_size](MergeInd const &a, MergeInd const &b) {
    return records[a.run_id * run_size + a.record_id % run_size] >
           records[b.run_id * run_size + b.record_id % run_size];
  };

  std::make_heap(begin, end, cmp);

  std::size_t out_ind = 0;
  bool first = true;
  while (begin != end) {
    out.out[out_ind++] =
        records[begin->run_id * run_size + begin->record_id % run_size];
    std::pop_heap(begin, end, cmp);
    if(out_ind == 1 && first) {
        Record_t::copy_rec(out.out[out_ind - 1], outputWitnessRecord);
        first = false;
    } else {
      (*outputWitnessRecord).x_or(out.out[out_ind - 1]);
    }

    if ((++((end - 1)->record_id)) % run_size == 0) {
      auto cur_iter = end - 1;
      if (cur_iter->record_id >= run_info.exrun_size) {
        --end;
      } else {
        dev.hd_in->eread(
            reinterpret_cast<char *>(
                (records + run_size * cur_iter->run_id).data()),
            run_size * Record_t::bytes,
            (run_info.exrun_size * cur_iter->run_id + cur_iter->record_id) *
                Record_t::bytes);
      }
    } // if

    std::push_heap(begin, end, cmp);
    if (out_ind == out.out_size) {
      // TODO: check return value
      dev.hd_out->eappend(reinterpret_cast<char *>(out.out.data()),
                          out.out_size * Record_t::bytes);
      out_ind = 0;
    } // if
  }
  if (out_ind > 0) {
    // TODO: check return value
    dev.hd_out->eappend(reinterpret_cast<char *>(out.out.data()),
                        out_ind * Record_t::bytes);
  } // if
} // external_merge
