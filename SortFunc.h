#pragma once

#include "Device.h"
#include "Iterator.h"
#include "Record.h"

struct RunInfo {
  RowCount const run_size; // number of records in a run
  RowCount const n_runs;
};

struct OutBuffer {
  RowCount out_size; // number of records in the output
  RecordArr_t &out;
};

struct DeviceInOut {
  Device *hd_in;
  Device *hd_out;
};

struct ExRunInfo : RunInfo {
  RowCount const exrun_size; // number of records in a device run
};

void incache_sort(RecordArr_t &records, Index_t &index,
                  RowCount const n_records);

void incache_sort(RecordArr_t const &records, RecordArr_t &out, Index_t &index,
                  RowCount const n_records);

void inmem_merge(RecordArr_t const &records, OutBuffer out, Device *hd,
                 Index_r &index, RunInfo run_info, RowCount *duplicatesCount);

void inmem_spill_merge(RecordArr_t &records, OutBuffer out, DeviceInOut dev,
                       Index_r &index, RunInfo run_info,
                       RowCount const n_runs_ssd, RowCount *duplicatesCount);

void external_merge(RecordArr_t &records, OutBuffer out, DeviceInOut dev,
                    Index_r &index, ExRunInfo run_info, RowCount *duplicatesCount);

void external_spill_merge(RecordArr_t &records, OutBuffer out, DeviceInOut dev,
                          Device *dev_exin, Index_r &index, ExRunInfo run_info,
                          RowCount const n_runs_hdd, RowCount *duplicatesCount);

inline void fill_run(Device *dev, RecordArr_t &out,
                     std::size_t const fill_records) {
  out[0].fill();
  uint64_t end_pos = dev->get_pos() + fill_records * Record_t::bytes;
  dev->eappend(out[0], Record_t::bytes);

  uint64_t const base = dev->get_base();
  dev->set_base(0);
  dev->ewrite(out[0], Record_t::bytes, end_pos - Record_t::bytes);

  dev->set_base(base);
}
