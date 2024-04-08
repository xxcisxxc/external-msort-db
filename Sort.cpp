#include "Sort.h"
#include "Device.h"
#include "Record.h"
#include "SortFunc.h"
#include "defs.h"
#include <climits>

SortPlan::SortPlan(Plan *const input)
    : _input(input), _rcache(input->records()), _icache(input->records()),
      _rmem(RecordArr_t(std::shared_ptr<Record_t>(
                            reinterpret_cast<Record_t *>(new char[kMemSize])),
                        kMemSize / Record_t::bytes)),
      ssd(std::make_unique<Device>("ssd", 0.1, 200, 10 * 1024)),
      hdd(std::make_unique<Device>("hdd", 5, 100, ULONG_MAX)) {
  TRACE(true);
} // SortPlan::SortPlan

SortPlan::~SortPlan() {
  TRACE(true);
  delete _input;
} // SortPlan::~SortPlan

Iterator *SortPlan::init() const {
  TRACE(true);
  return new SortIterator(this);
} // SortPlan::init

SortIterator::SortIterator(SortPlan const *const plan)
    : _plan(plan), _input(plan->_input->init()), _consumed(0), _produced(0),
      _kRowCacheRun(cache_nrecords()),
      _kRowMemRun((kMemSize - kCacheSize) / Record_t::bytes),
      _kRowMemOut(kCacheSize / Record_t::bytes) {
  TRACE(true);
} // SortIterator::SortIterator

SortIterator::~SortIterator() {
  TRACE(true);

  delete _input;
  traceprintf("produced %lu of %lu rows\n", (unsigned long)(_produced),
              (unsigned long)(_consumed));
} // SortIterator::~SortIterator

bool SortIterator::next() {
  TRACE(true);
  static uint64_t mem_offset = 0;

  RecordArr_t records = _plan->_rcache.records;
  records += _consumed;
  Index_t indext = _plan->_rcache.index;

  do {
    if (!_input->next()) {
      break;
    }
    ++_consumed;
  } while (_consumed % 8 != 0); // run_size: 8 (cache-sized run)

  if (_produced >= _consumed) {
    // TODO: merge all the unmerged runs
    // 1. Merge remaining runs in mem to ssd
    // 2. Merge remaining runs in ssd to hdd
    // 3. Merge all the runs in hdd (maybe nested)
    return false;
  }

  RecordArr_t work = _plan->_rmem.work + mem_offset;
  incache_sort(records, work, indext, _consumed - _produced);
  mem_offset += _consumed - _produced;
  _produced = _consumed;

  if (_consumed % 32 == 0) { // run_size * n_runs: 8 * 4 (memory-sized run)
    Index_r indexr = _plan->_icache.index;
    RecordArr_t in = _plan->_rmem.work;
    RecordArr_t out = _plan->_rmem.out;
    Device *ssd = _plan->ssd.get();
    inmem_merge(in, {2 * 8, out}, ssd, indexr, {8, 4});
    mem_offset = 0;
  }

  if (_consumed % 256 == 0) { // run_size * n_runs: 32 * 8 (ssd-sized run)
    Index_r indexr = _plan->_icache.index;
    RecordArr_t in = _plan->_rmem.work;
    RecordArr_t out = _plan->_rmem.out;
    Device *ssd = _plan->ssd.get();
    Device *hdd = _plan->hdd.get();

    // load 4 records from each 8 runs in sdd
    external_merge(in, {2 * 8, out}, {ssd, hdd}, indexr, {{4, 8}, 32});
    ssd->clear();
  } // if ssd is full, load from ssd merge to hdd

  return true;
} // SortIterator::next
