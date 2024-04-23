#include "SortFunc.h"
#include "LoserTree.h"
#include "Record.h"
#include "defs.h"
#include <algorithm>
#include <climits>
#include <cmath>
#include <cstddef>
#include <cstdio>

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
                 Index_r &index, RunInfo run_info) {
  RowCount const run_size = run_info.run_size;
  RowCount const n_runs = run_info.n_runs;

  Level level(ceil(log2(n_runs)));
  uint64_t capacity = 1 << level;
  auto end = index.begin() + capacity;
  if (end > index.end()) {
    throw std::out_of_range("inmem_merge: index out of range");
  } // if

  auto get_record = [&records,
                     &run_size](MergeInd const &mind) -> Record_t const & {
    return records[mind.run_id * run_size + mind.record_id];
  };

  auto cmp = [&get_record, &n_runs](MergeInd const &a, MergeInd const &b) {
    if (a.run_id >= n_runs) {
      return false;
    } else if (b.run_id >= n_runs) {
      return true;
    }
    return get_record(a) < get_record(b);
  };

  LoserTree ltree(level, cmp, index);
  for (uint32_t i = 0; i < capacity; ++i) {
    ltree.insert(i, 0);
  }

  std::size_t out_ind = 0;

  while (!ltree.empty()) {
    MergeInd popped = ltree.pop();
    const Record_t &rec = get_record(popped);
    if (popped.run_id >= n_runs || rec.isfilled()) {
      ltree.deleteRecordId(popped.run_id);
      continue;
    }
    out.out[out_ind++] = rec;
    // for maintaining the witness record

    ++popped.record_id;

    if (popped.record_id < run_size) {
      ltree.insert(popped.run_id, popped.record_id);
    } else {
      ltree.deleteRecordId(popped.run_id);
    }
    // delete &popped;
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

void inmem_spill_merge(RecordArr_t &records, OutBuffer out, DeviceInOut dev,
                       Index_r &index, RunInfo run_info,
                       RowCount const n_runs_ssd) {
  RowCount const mem_run_size = run_info.run_size;
  RowCount const n_runs = run_info.n_runs + n_runs_ssd;
  RowCount const ssd_run_size =
      run_info.run_size / 2; // TODO: make cache run size even

  // make space for the ssd runs
  for (uint32_t i = 0; i < n_runs_ssd; ++i) {
    char *exchange = records[i * mem_run_size + ssd_run_size];
    dev.hd_in->eappend(exchange, ssd_run_size * Record_t::bytes);
    dev.hd_in->eread(exchange, ssd_run_size * Record_t::bytes,
                     (i * mem_run_size) * Record_t::bytes);
  } // for

  Level level(ceil(log2(n_runs)));
  uint64_t capacity = 1 << level;
  auto end = index.begin() + capacity;
  if (end > index.end()) {
    throw std::out_of_range("inmem_spill: index out of range");
  } // if

  auto get_record = [&records, &mem_run_size, &ssd_run_size,
                     &n_runs_ssd](MergeInd const &mind) -> Record_t const & {
    if (mind.run_id < 2 * n_runs_ssd) {
      return records[mind.run_id * ssd_run_size +
                     mind.record_id % ssd_run_size];
    } else {
      return records[2 * n_runs_ssd * ssd_run_size +
                     (mind.run_id - 2 * n_runs_ssd) * mem_run_size +
                     mind.record_id];
    }
  };

  auto cmp = [&n_runs, &get_record](MergeInd const &a, MergeInd const &b) {
    if (a.run_id >= n_runs) {
      return false;
    } else if (b.run_id >= n_runs) {
      return true;
    }
    return get_record(a) < get_record(b);
  };

  LoserTree ltree(level, cmp, index);
  for (uint32_t i = 0; i < capacity; ++i) {
    ltree.insert(i, 0);
  }

  std::size_t out_ind = 0;

  while (!ltree.empty()) {
    MergeInd popped = ltree.pop();
    const Record_t &rec = get_record(popped);
    if (popped.run_id >= n_runs || rec.isfilled()) {
      ltree.deleteRecordId(popped.run_id);
      continue;
    }
    out.out[out_ind++] = rec;

    ++popped.record_id;

    if (popped.run_id < 2 * n_runs_ssd &&
        popped.record_id % ssd_run_size == 0) {
      // uint64_t offset = ((popped.run_id / 2) * mem_run_size) *
      // Record_t::bytes;
      uint64_t offset =
          popped.run_id % 2
              ? ((popped.run_id / 2) * mem_run_size) + ssd_run_size
              : mem_run_size * n_runs_ssd +
                    ((popped.run_id / 2) * ssd_run_size);
      offset *= Record_t::bytes;
      dev.hd_in->eread(
          reinterpret_cast<char *>(&records[popped.run_id * ssd_run_size]),
          Record_t::bytes * ssd_run_size, offset);
    }

    if (popped.record_id < mem_run_size) {
      ltree.insert(popped.run_id, popped.record_id);
    } else {
      ltree.deleteRecordId(popped.run_id);
    }
    // delete &popped;
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
} // inmem_spill_merge

void external_merge(RecordArr_t &records, OutBuffer out, DeviceInOut dev,
                    Index_r &index, ExRunInfo run_info) {
  RowCount const run_size = run_info.run_size;
  RowCount const n_runs = run_info.n_runs;

  // int count = 0;
  for (uint32_t run_id = 0; run_id < n_runs; ++run_id) { // n_runs in sdd: 8
    dev.hd_in->eread(
        reinterpret_cast<char *>((records + run_size * run_id).data()),
        run_size * Record_t::bytes,
        (run_info.exrun_size * run_id) * Record_t::bytes);
  } // for

  Level level(ceil(log2(n_runs)));
  uint64_t capacity = 1 << level;
  auto end = index.begin() + capacity;
  if (end > index.end()) {
    throw std::out_of_range("external_merge: index out of range");
  } // if

  auto get_record = [&records, &run_size](MergeInd const &mind) -> Record_t & {
    return records[mind.run_id * run_size + mind.record_id % run_size];
  };

  auto cmp = [&get_record, &n_runs](MergeInd const &a, MergeInd const &b) {
    if (a.run_id >= n_runs) {
      return false;
    } else if (b.run_id >= n_runs) {
      return true;
    }
    return get_record(a) < get_record(b);
  };

  LoserTree ltree(level, cmp, index);
  for (uint32_t i = 0; i < capacity; ++i) {
    ltree.insert(i, 0);
  }

  std::size_t out_ind = 0;

  while (!ltree.empty()) {
    MergeInd popped = ltree.pop();
    const Record_t &rec = get_record(popped);
    if (popped.run_id >= n_runs || rec.isfilled()) {
      ltree.deleteRecordId(popped.run_id);
      continue;
    }

    out.out[out_ind++] = rec;

    ++popped.record_id;
    if (popped.record_id % run_size == 0) {
      RowCount maxToBeRead = run_size;
      if (popped.record_id + run_size > run_info.exrun_size) {
        maxToBeRead = run_info.exrun_size - popped.record_id;
      }
      dev.hd_in->eread(
          reinterpret_cast<char *>((records + run_size * popped.run_id).data()),
          maxToBeRead * Record_t::bytes,
          (run_info.exrun_size * popped.run_id + popped.record_id) *
              Record_t::bytes);
      //}
    }
    // if
    if (popped.record_id < run_info.exrun_size) {
      ltree.insert(popped.run_id, popped.record_id);
    } else {
      ltree.deleteRecordId(popped.run_id);
    }
    if (out_ind == out.out_size) {
      // TODO: check return value
      dev.hd_out->eappend(reinterpret_cast<char *>(out.out.data()),
                          out.out_size * Record_t::bytes);
      printf("out_ind: %lu\n", out_ind);
      out_ind = 0;
    } // if
  }
  if (out_ind > 0) {
    // TODO: check return value
    dev.hd_out->eappend(reinterpret_cast<char *>(out.out.data()),
                        out_ind * Record_t::bytes);
    printf("out_ind: %lu\n", out_ind);
  } // if
} // external_merge

void external_spill_merge(RecordArr_t &records, OutBuffer out, DeviceInOut dev,
                          Device *dev_exin, Index_r &index, ExRunInfo run_info,
                          RowCount const n_runs_hdd) {
  RowCount const run_size = run_info.run_size;
  RowCount const n_runs = run_info.n_runs + n_runs_hdd;

  // int count = 0;
  for (uint32_t run_id = 0; run_id < run_info.n_runs;
       ++run_id) { // n_runs in sdd: 8
    dev.hd_in->eread(
        reinterpret_cast<char *>((records + run_size * run_id).data()),
        run_size * Record_t::bytes,
        (run_info.exrun_size * run_id) * Record_t::bytes);
  } // for
  for (uint32_t run_id = run_info.n_runs; run_id < n_runs; ++run_id) {
    dev_exin->eread(
        reinterpret_cast<char *>((records + run_size * run_id).data()),
        run_size * Record_t::bytes,
        (run_info.exrun_size * (run_id - run_info.n_runs)) * Record_t::bytes);
  }

  Level level(ceil(log2(n_runs)));
  uint64_t capacity = 1 << level;
  auto end = index.begin() + capacity;
  if (end > index.end()) {
    throw std::out_of_range("external_merge: index out of range");
  } // if

  auto get_record = [&records, &run_size](MergeInd const &mind) -> Record_t & {
    return records[mind.run_id * run_size + mind.record_id % run_size];
  };

  auto cmp = [&get_record, &n_runs](MergeInd const &a, MergeInd const &b) {
    if (a.run_id >= n_runs) {
      return false;
    } else if (b.run_id >= n_runs) {
      return true;
    }
    return get_record(a) < get_record(b);
  };

  LoserTree ltree(level, cmp, index);
  for (uint32_t i = 0; i < capacity; ++i) {
    ltree.insert(i, 0);
  }

  std::size_t out_ind = 0;

  while (!ltree.empty()) {
    MergeInd popped = ltree.pop();
    const Record_t &rec = get_record(popped);
    if (popped.run_id >= n_runs || rec.isfilled()) {
      ltree.deleteRecordId(popped.run_id);
      continue;
    }

    out.out[out_ind++] = rec;

    ++popped.record_id;
    if (popped.record_id % run_size == 0) {
      RowCount maxToBeRead = run_size;
      if (popped.record_id + run_size > run_info.exrun_size) {
        maxToBeRead = run_info.exrun_size - popped.record_id;
      }
      if (popped.run_id < run_info.n_runs) {
        dev.hd_in->eread(
            reinterpret_cast<char *>(
                (records + run_size * popped.run_id).data()),
            maxToBeRead * Record_t::bytes,
            (run_info.exrun_size * popped.run_id + popped.record_id) *
                Record_t::bytes);
      } else {
        dev_exin->eread(
            reinterpret_cast<char *>(
                (records + run_size * popped.run_id).data()),
            maxToBeRead * Record_t::bytes,
            (run_info.exrun_size * (popped.run_id - run_info.n_runs) +
             popped.record_id) *
                Record_t::bytes);
      }
    }
    // if
    if (popped.record_id < run_info.exrun_size) {
      ltree.insert(popped.run_id, popped.record_id);
    } else {
      ltree.deleteRecordId(popped.run_id);
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
