#include "Sort.h"
#include "Consts.h"
#include "Device.h"
#include "Record.h"
#include "SortFunc.h"
#include "Utils.h"
#include "defs.h"
#include <climits>

SortPlan::SortPlan(Plan *const input)
    : _input(input), _rcache(input->records()), _icache(input->records()),
      _rmem(RecordArr_t(std::shared_ptr<Record_t>(
                            reinterpret_cast<Record_t *>(new char[kMemSize])),
                        kMemSize / Record_t::bytes)),
      ssd(std::make_unique<Device>(kSSD, 0.1, 200, 10 * 1024)),
      hdd(std::make_unique<Device>(kHDD, 5, 100, ULONG_MAX)),
      hddout(std::make_unique<Device>(kOut, 5, 100, ULONG_MAX)),
      _inputWitnessRecord(_input->witnessRecord()) {
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
  static bool finished = false;

  if (finished) {
    return false;
  }

  RecordArr_t records = _plan->_rcache.records;
  // records += _consumed;
  Index_t indext = _plan->_rcache.index;

  do {
    if (!_input->next()) {
      break;
    }
    ++_consumed;
  } while ((_consumed % cache_run_nrecords()) !=
           0); // run_size: 8 (cache-sized run)

  Index_r indexr = _plan->_icache.index;
  RecordArr_t in = _plan->_rmem.work;
  RecordArr_t out = _plan->_rmem.out;
  Device *ssd = _plan->ssd.get();
  Device *hdd = _plan->hdd.get();
  Device *hddout = _plan->hddout.get();

  if (_produced >= _consumed) {
    if (dram_nrecords() < _consumed && _consumed < 2 * dram_nrecords()) {
      // traceprintf("spill merge: n_runs_ssd: %lu\n",
      //             (_consumed - dram_nrecords() + 7) / 8);
      inmem_spill_merge(in, {2 * 8, out}, {ssd, hddout}, indexr, {8, 4},
                        (_consumed - dram_nrecords() + 7) / 8);
    }
    finished = true;
    return false;
    // sort all cache runs to inmemory

    Index_r indexr = _plan->_icache.index;
    RecordArr_t in = _plan->_rmem.work;
    RecordArr_t out = _plan->_rmem.out;
    Device *ssd = _plan->ssd.get();
    RowCount inmemRemaining = _consumed % dram_nrecords();
    RowCount inmemRuns = (inmemRemaining / cache_run_nrecords()) +
                         ((inmemRemaining % cache_run_nrecords()) == 0 ? 0 : 1);
    RowCount lastRunLimit =
        inmemRemaining -
        ((inmemRemaining / cache_run_nrecords()) * cache_run_nrecords());
    if (inmemRemaining != 0) {
      inmem_merge(in, {2 * 8, out}, ssd, indexr,
                  {cache_run_nrecords(), inmemRuns});
    }

    // sort all ssd runs to hdd
    Device *hdd = _plan->hdd.get();
    Device *hddout = _plan->hddout.get();

    RowCount ssdRemaining = _consumed % ssd_nrecords();
    RowCount ssdRuns = (ssdRemaining / dram_nrecords()) +
                       ((ssdRemaining % dram_nrecords()) == 0 ? 0 : 1);
    RowCount ssdLastRunLimit =
        ssdRemaining - ((ssdRemaining / dram_nrecords()) * dram_nrecords());
    // load 4 records from each 8 runs in sdd
    if (ssdRemaining != 0) {
      external_merge(in, {2 * 8, out}, {ssd, hdd}, indexr,
                     {{4, ssdRuns}, dram_nrecords()}, ssdLastRunLimit);
    }
    ssd->clear();

    // sort all hdd runs to hdd
    RowCount totalRecords = (hdd->getConsumption()) / (Record_t::bytes);
    RowCount hddRuns = (totalRecords / ssd_nrecords()) +
                       ((totalRecords % ssd_nrecords()) == 0 ? 0 : 1);
    RowCount hddLastRunLimit =
        totalRecords - (totalRecords / ssd_nrecords()) * ssd_nrecords();
    if (hddRuns > 1) {
      external_merge(in, {2 * 8, out}, {hdd, hddout}, indexr,
                     {{4, hddRuns}, ssd_nrecords()}, hddLastRunLimit);
    }

    // TODO: merge all the unmerged runs
    // 1. Merge remaining runs in mem to ssd
    // 2. Merge remaining runs in ssd to hdd
    // 3. Merge all the runs in hdd (maybe nested)
    return false;
  }

  if (dram_nrecords() < _consumed && _consumed <= 2 * dram_nrecords()) {
    // spill one cache run to ssd
    // mem_size < _consumed < 2 * mem_size
    ssd->eappend(reinterpret_cast<char *>((in + mem_offset).data()),
                 cache_run_nrecords() * Record_t::bytes);
  }

  RecordArr_t work = _plan->_rmem.work + mem_offset;
  incache_sort(records, work, indext, _consumed - _produced);
  mem_offset += _consumed - _produced;
  if (_consumed % cache_run_nrecords() != 0) {
    for (std::size_t i = _consumed % cache_run_nrecords();
         i < cache_run_nrecords(); ++i) {
      work[i].fill(CHAR_MAX);
    }
  }
  _produced = _consumed;

  // run_size * n_runs: 8 * 4 (memory-sized run)
  if (_consumed % dram_nrecords() == 0) {
    mem_offset = 0;

    if (_consumed >= 2 * dram_nrecords()) {
      // merge all cache runs in memory to ssd
      if (_consumed == ssd_nrecords()) {
        // first run is clear because of spilling
        ssd->clear();
      }
      inmem_merge(in, {2 * 8, out}, ssd, indexr, {8, 4});
    }
    if (_consumed == 2 * dram_nrecords()) {
      // merge all inmemory runs to ssd
      ssd->eread(reinterpret_cast<char *>(in.data()),
                 dram_nrecords() * Record_t::bytes, 0);
      inmem_merge(in, {2 * 8, out}, ssd, indexr, {8, 4});
    }

    // TODO: spilling: if mem_size < _consumed < 2 * mem_size, spill one cache
    // run to ssd
    // TODO: spilling: if _consumed == 2 * mem_size, merge all cache to ssd,
    // merge unmerged first run
    // TODO: if goes final merge step before _consumed >= 2 * mem_size
  }

  if (_consumed % ssd_nrecords() ==
      0) { // run_size * n_runs: 32 * 8 (ssd-sized run)
    Index_r indexr = _plan->_icache.index;
    RecordArr_t in = _plan->_rmem.work;
    RecordArr_t out = _plan->_rmem.out;
    Device *ssd = _plan->ssd.get();

    // load 4 records from each 8 runs in sdd
    external_merge(in, {2 * 8, out}, {ssd, hdd}, indexr, {{4, 8}, 32}, 0);
    ssd->clear();
    // TODO:: move this to final merge place
    //  if(*(_plan->_input->witnessRecord()) == *(_plan->witnessRecord()))
    //    traceprintf("yess\n");
    //  else
    //   traceprintf("NOOO\n");
  } // if ssd is full, load from ssd merge to hdd

  return true;
} // SortIterator::next
