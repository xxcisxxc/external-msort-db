#include "Sort.h"
#include "Consts.h"
#include "Device.h"
#include "Record.h"
#include "SortFunc.h"
#include "Utils.h"
#include "defs.h"
#include <climits>
#include <cstdint>
#include <sys/types.h>

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

  do {
    if (!_input->next()) {
      break;
    }
    ++_consumed;
  } while ((_consumed % cache_nrecords()) !=
           0); // run_size: 8 (cache-sized run)

  Index_r indexr = _plan->_icache.index;
  RecordArr_t in = _plan->_rmem.work;
  RecordArr_t out = _plan->_rmem.out;
  Device *ssd = _plan->ssd.get();
  Device *hdd = _plan->hdd.get();
  Device *hddout = _plan->hddout.get();

  if (_produced >= _consumed) {
    // final merge step
    if (_consumed <= mem_nrecords()) {
      // memory is not full, merge all cache-sized runs in memory to out
      inmem_merge(in, {2 * 8, out}, hddout, indexr,
                  {cache_nrecords(),
                   (_consumed + cache_nrecords() - 1) / cache_nrecords()});
      finished = true;
      return false;
    }
    if (mem_nrecords() < _consumed && _consumed < 2 * mem_nrecords()) {
      // input ends during spilling mem->ssd
      inmem_spill_merge(in, {2 * 8, out}, {ssd, hddout}, indexr, {8, 4},
                        (_consumed - mem_nrecords() + cache_nrecords() - 1) /
                            cache_nrecords());
      finished = true;
      return false;
    }

    RowCount inmem_rem = _consumed % mem_nrecords();
    if (inmem_rem != 0) {
      if (ssd_nrecords() < _consumed && _consumed <= 2 * ssd_nrecords()) {
        // spill runs from ssd to hdd
        inmem_merge(in, {2 * 8, out}, hdd, indexr,
                    {cache_nrecords(),
                     (inmem_rem + cache_nrecords() - 1) / cache_nrecords()});
      } else {
        // merge remaining runs in memory to ssd (create the last run in ssd)
        inmem_merge(in, {2 * 8, out}, ssd, indexr,
                    {cache_nrecords(),
                     (inmem_rem + cache_nrecords() - 1) / cache_nrecords()});
        // fill the last run in ssd
        out[0].fill();
        ssd->eappend(out[0], Record_t::bytes);
        uint64_t end_pos = ssd->get_pos() + mem_nrecords() - inmem_rem;
        ssd->ewrite(out[0], Record_t::bytes, (end_pos - 1) * Record_t::bytes);
      }
    }

    if (_consumed <= ssd_nrecords()) {
      // input ends with multiple runs in ssd (none in hdd), merge to out
      // TODO: calculate run size
      external_merge(in, {2 * 8, out}, {ssd, hddout}, indexr,
                     {{4, (_consumed + mem_nrecords() - 1) / mem_nrecords()},
                      mem_nrecords()});
      finished = true;
      return false;
    }

    if (ssd_nrecords() < _consumed && _consumed < 2 * ssd_nrecords()) {
      external_spill_merge(
          in, {2 * 8, out}, {ssd, hddout}, hdd, indexr, {{2, 8}, 32},
          (_consumed - ssd_nrecords() + mem_nrecords() - 1) / mem_nrecords());
      finished = true;
      return false;
    }

    // merge all ssd runs to hdd
    external_merge(in, {2 * 8, out}, {hdd, hddout}, indexr,
                   {{4, (_consumed + ssd_nrecords() - 1) / ssd_nrecords()},
                    ssd_nrecords()});
    finished = true;
    return false;
    // TODO: nested merge
  }

  if (mem_nrecords() < _consumed && _consumed <= 2 * mem_nrecords()) {
    // spilling mem->ssd: dump the candidate cache run to ssd
    ssd->eappend(reinterpret_cast<char *>((in + mem_offset).data()),
                 cache_nrecords() * Record_t::bytes);
  }

  // sort cache run and dump to memory
  RecordArr_t records = _plan->_rcache.records;
  Index_t indext = _plan->_rcache.index;

  RecordArr_t work = _plan->_rmem.work + mem_offset;
  incache_sort(records, work, indext, _consumed - _produced);
  mem_offset += _consumed - _produced;
  if (_consumed % cache_nrecords() != 0) {
    // last cache run is not full, fill
    work[_consumed % cache_nrecords()].fill();
  }
  _produced = _consumed;

  if (_consumed % mem_nrecords() == 0) {
    // when memory is full

    mem_offset = 0; // reset offset

    if (_consumed >= 2 * mem_nrecords()) {
      // not spilling, eager merge mem runs to ssd
      if (ssd_nrecords() < _consumed && _consumed <= 2 * ssd_nrecords()) {
        // spill runs from ssd to hdd
        inmem_merge(in, {2 * 8, out}, hdd, indexr, {8, 4});
      } else {
        // merge all cache-sized runs in memory to ssd
        inmem_merge(in, {2 * 8, out}, ssd, indexr,
                    {cache_nrecords(), mem_nruns()});
      }
    }
    if (_consumed == 2 * mem_nrecords()) {
      // merge the first block of unmerged runs in ssd due to spilling
      ssd->eread(reinterpret_cast<char *>(in.data()),
                 mem_nrecords() * Record_t::bytes, 0);
      uint64_t cur_pos = ssd->get_pos();
      ssd->clear();
      inmem_merge(in, {2 * 8, out}, ssd, indexr,
                  {cache_nrecords(), mem_nruns()});
      ssd->eseek(cur_pos);
    }
  }

  // run_size * n_runs: 32 * 8 (ssd-sized run)
  if (_consumed % ssd_nrecords() == 0) {
    if (_consumed >= 2 * ssd_nrecords()) {
      // merge all ssd runs to hdd
      external_merge(in, {2 * 8, out}, {ssd, hdd}, indexr, {{4, 8}, 32});
    }
    if (_consumed == 2 * ssd_nrecords()) {
      // merge all spilled ssd runs to hdd
      external_merge(in, {2 * 8, out}, {hdd, hdd}, indexr, {{4, 8}, 32});
    }
    // load 4 records from each 8 runs in sdd
    ssd->clear();
    // TODO:: move this to final merge place
    //  if(*(_plan->_input->witnessRecord()) == *(_plan->witnessRecord()))
    //    traceprintf("yess\n");
    //  else
    //   traceprintf("NOOO\n");
  } // if ssd is full, load from ssd merge to hdd

  return true;
} // SortIterator::next
