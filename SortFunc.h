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
                 Index_r &index, RunInfo run_info, Record_t *outputWitnessRecord);

void external_merge(RecordArr_t &records, OutBuffer out, DeviceInOut dev,
                    Index_r &index, ExRunInfo run_info, Record_t *outputWitnessRecord);
