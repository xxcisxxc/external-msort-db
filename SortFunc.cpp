#include "SortFunc.h"
#include "Record.h"
#include <algorithm>
#include <cstddef>
#include "LoserTree.cpp"
#include <cmath>

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

  auto begin = index.begin();
  auto end = index.begin() + n_runs;
  if (end > index.end()) {
    throw std::out_of_range("inmem_merge: index out of range");
  } // if
  Level level(ceil(log2(n_runs)));

  uint32_t capacity = 1<<level;

  auto cmp = [&records, &run_size, &n_runs, &capacity](MergeInd const &a, MergeInd const &b) {
    if(a.run_id >= n_runs ) {
      return false;
    } else if(b.run_id >= n_runs) {
      return true;
    }
    return records[a.run_id * run_size + a.record_id] >
           records[b.run_id * run_size + b.record_id];
  };

  LoserTree ltree(level, cmp, index);
  for (uint32_t i = 0; i < capacity; ++i) {
    ltree.insert(i, 0);
  }

  std::size_t out_ind = 0;
  bool first = true;

  while (!ltree.empty())
  {
    MergeInd popped = ltree.pop();
    if(popped.run_id >=run_size) {
      ltree.deleteRecordId(popped.run_id);
      continue;
    }
    uint32_t index = popped.run_id * run_size + popped.record_id;
    std::cout<<"index is - " <<index<<std::endl;
    out.out[out_ind++] = records[popped.run_id * run_size + popped.record_id];
    // for maintaining the witness record
    if(out_ind == 1 && first) {
        Record_t::copy_rec(out.out[out_ind - 1], outputWitnessRecord);
        first = false;
    } else {
        (*outputWitnessRecord).x_or(out.out[out_ind - 1]);
    }
    if(++popped.record_id < run_size){
      ltree.insert(popped.run_id, popped.record_id);
    } else {
      ltree.deleteRecordId(popped.run_id);
    }
    //delete &popped;
     if (out_ind == out.out_size) {
      // TODO: check return value
      hd->eappend(reinterpret_cast<char *>(out.out.data()),
                  out.out_size * Record_t::bytes);
      out_ind = 0;
    } // if
    /* code */
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

  int count = 0;
  for (uint32_t run_id = 0; run_id < n_runs; ++run_id) { // n_runs in sdd: 8
    dev.hd_in->eread(
        reinterpret_cast<char *>((records + run_size * run_id).data()),
        run_size * Record_t::bytes,
        (run_info.exrun_size * run_id) * Record_t::bytes);
  } // for


  Level level(ceil(log2(n_runs)));
  uint32_t capacity = 1<<level;

  auto cmp = [&records, &run_size, &run_info, &n_runs](MergeInd const &a, MergeInd const &b) {
    if(a.run_id >= n_runs) {
      return false;
    } else if(b.run_id >= n_runs ) {
      return true;
    }
    return records[a.run_id * run_size + a.record_id % run_size] >
           records[b.run_id * run_size + b.record_id % run_size];
  };

  LoserTree ltree(level, cmp, index);
    for (uint32_t i = 0; i < capacity; ++i) {
      ltree.insert(i, 0);
    }

  std::size_t out_ind = 0;
  bool first = true;
  
  while (!ltree.empty()) {
    MergeInd popped = ltree.pop();
    if(popped.run_id >= n_runs) {
      ltree.deleteRecordId(popped.run_id);
      continue;
    }
    count++;
    uint32_t index = popped.run_id * run_info.exrun_size + popped.record_id;
    out.out[out_ind++] =
        records[popped.run_id * run_size + popped.record_id % run_size];
    //std::pop_heap(begin, end, cmp);
    if(out_ind == 1 && first) {
        Record_t::copy_rec(out.out[out_ind - 1], outputWitnessRecord);
        first = false;
    } else {
      (*outputWitnessRecord).x_or(out.out[out_ind - 1]);
    }

    if (++popped.record_id % run_size == 0) {
      //auto cur_iter = end - 1;
      if (popped.record_id >= run_info.exrun_size) {
        ltree.deleteRecordId(popped.run_id);
        //--end;
      } else {
        dev.hd_in->eread(
            reinterpret_cast<char *>(
                (records + run_size * popped.run_id).data()),
            run_size * Record_t::bytes,
            (run_info.exrun_size * popped.run_id + popped.record_id) *
                Record_t::bytes);
      }
    } // if
    if (popped.record_id < run_info.exrun_size) {
        ltree.insert(popped.run_id, popped.record_id);
    }
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
