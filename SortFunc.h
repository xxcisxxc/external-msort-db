#pragma once

#include "Iterator.h"
#include "Record.h"

void incache_sort(RecordArr_t &records, Index_t &index,
                  RowCount const n_records);

void incache_sort(RecordArr_t const &records, RecordArr_t &out, Index_t &index,
                  RowCount const n_records);

void inmem_merge(RecordArr_t const &records, RecordArr_t &out, Index_r &index,
                 RowCount const run_size, RowCount const n_runs);
