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
      inmem_merge(in, {out_nrecords(), out}, hddout, indexr,
                  {cache_nrecords(),
                   (_consumed + cache_nrecords() - 1) / cache_nrecords()});
      finished = true;
      return false;
    }
    if (mem_nrecords() < _consumed && _consumed < 2 * mem_nrecords()) {
      // input ends during spilling mem->ssd
      inmem_spill_merge(in, {out_nrecords(), out}, {ssd, hddout}, indexr,
                        {cache_nrecords(), mem_nruns()},
                        (_consumed - mem_nrecords() + cache_nrecords() - 1) /
                            cache_nrecords());
      finished = true;
      return false;
    }

    RowCount inmem_rem = _consumed % mem_nrecords();
    if (inmem_rem != 0) {
      // hdd: spill from ssd to hdd
      // ssd: merge remaining runs in memory to ssd
      Device *out_dev =
          (ssd_nrecords() < _consumed && _consumed <= 2 * ssd_nrecords()) ? hdd
                                                                          : ssd;

      // merge remaining runs in memory to ssd (create the last run in ssd)
      inmem_merge(in, {out_nrecords(), out}, out_dev, indexr,
                  {cache_nrecords(),
                   (inmem_rem + cache_nrecords() - 1) / cache_nrecords()});
      // fill the last run in ssd
      fill_run(out_dev, out, mem_nrecords() - inmem_rem);
    }

    if (_consumed <= ssd_nrecords()) {
      // input ends with multiple runs in ssd (none in hdd), merge to out
      uint32_t n_runs = (_consumed + mem_nrecords() - 1) / mem_nrecords();
      uint32_t run_size = mmem_nrecords() / n_runs;
      external_merge(in, {out_nrecords(), out}, {ssd, hddout}, indexr,
                     {{run_size, n_runs}, mem_nrecords()});
      finished = true;
      return false;
    }

    if (ssd_nrecords() < _consumed && _consumed < 2 * ssd_nrecords()) {
      uint32_t n_runs_hdd =
          (_consumed - ssd_nrecords() + mem_nrecords() - 1) / mem_nrecords();
      uint32_t run_size = mmem_nrecords() / (ssd_nruns() + n_runs_hdd);
      external_spill_merge(in, {out_nrecords(), out}, {ssd, hddout}, hdd,
                           indexr, {{run_size, ssd_nruns()}, mem_nrecords()},
                           n_runs_hdd);
      finished = true;
      return false;
    }

    RowCount ssd_rem = _consumed % ssd_nrecords();
    if (ssd_rem != 0) {
      // merge remaining runs in ssd to hdd (create the last run in hdd)
      uint32_t n_runs = (ssd_rem + mem_nrecords() - 1) / mem_nrecords();
      uint32_t run_size = mmem_nrecords() / n_runs;
      external_merge(in, {out_nrecords(), out}, {ssd, hdd}, indexr,
                     {{run_size, n_runs}, mem_nrecords()});
      // fill the last run in hdd
      fill_run(hdd, out, mem_nrecords() - ssd_rem);
    }

    // merge all the remaining hdd runs to hddout
    uint32_t exrun_size, n_runs, run_size, n_subruns;
    exrun_size = ssd_nrecords();
    n_runs = (_consumed + exrun_size - 1) / exrun_size;
    run_size = mmem_nrecords() / n_runs;
    if (run_size < 4) { // TODO: setup a better threshold
      run_size = 4;
    }
    n_subruns = mmem_nrecords() / run_size;
    while (n_subruns < n_runs) {
      for (uint32_t i = 0; i < n_runs; i += n_subruns) {
        external_merge(
            in, {out_nrecords(), out}, {hdd, hdd}, indexr,
            {{run_size, (i + n_subruns > n_runs) ? n_runs - i : n_subruns},
             exrun_size});
        if (i + n_subruns > n_runs) {
          uint32_t rem_size = (i + n_subruns - n_runs) * exrun_size;
          fill_run(hdd, out, rem_size);

          hdd->set_base(n_runs * exrun_size * Record_t::bytes);
        } else {
          hdd->set_base((i + n_subruns) * exrun_size * Record_t::bytes);
        }
      }
      exrun_size = n_subruns * exrun_size;
      n_runs = (_consumed + exrun_size - 1) / exrun_size;
      run_size = mmem_nrecords() / n_runs;
      if (run_size < 4) { // TODO: setup a better threshold
        run_size = 4;
      }
      n_subruns = mmem_nrecords() / run_size;
    }
    external_merge(in, {out_nrecords(), out}, {hdd, hddout}, indexr,
                   {{run_size, n_runs}, exrun_size});
    finished = true;
    return false;
  } // if produced >= consumed

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
      Device *out_dev =
          (ssd_nrecords() < _consumed && _consumed <= 2 * ssd_nrecords()) ? hdd
                                                                          : ssd;

      // out_dev=ssd: merge all cache-sized runs in memory to ssd
      // out_dev=hdd: spill from ssd to hdd
      inmem_merge(in, {out_nrecords(), out}, out_dev, indexr,
                  {cache_nrecords(), mem_nruns()});
    }
    if (_consumed == 2 * mem_nrecords()) {
      // merge the first block of unmerged runs in ssd due to spilling
      ssd->eread(reinterpret_cast<char *>(in.data()),
                 mem_nrecords() * Record_t::bytes, 0);
      uint64_t cur_pos = ssd->get_pos();
      ssd->clear();
      inmem_merge(in, {out_nrecords(), out}, ssd, indexr,
                  {cache_nrecords(), mem_nruns()});
      ssd->eseek(cur_pos);
    }
  } // if

  if (_consumed % ssd_nrecords() == 0) {
    // when ssd is full

    uint32_t run_size = mmem_nrecords() / ssd_nruns();
    if (_consumed >= 2 * ssd_nrecords()) {
      // merge all ssd runs to hdd
      external_merge(in, {out_nrecords(), out}, {ssd, hdd}, indexr,
                     {{run_size, ssd_nruns()}, mem_nrecords()});
      ssd->clear();
    }
    if (_consumed == 2 * ssd_nrecords()) {
      // merge all spilled ssd runs to hdd (first to ssd then bulk write to hdd)
      external_merge(in, {out_nrecords(), out}, {hdd, ssd}, indexr,
                     {{run_size, ssd_nruns()}, mem_nrecords()});
      for (uint32_t i = 0; i < ssd_nruns(); ++i) {
        ssd->eread(reinterpret_cast<char *>(in.data()),
                   mem_nrecords() * Record_t::bytes,
                   i * mem_nrecords() * Record_t::bytes);
        hdd->ewrite(reinterpret_cast<char *>(in.data()),
                    mem_nrecords() * Record_t::bytes,
                    i * mem_nrecords() * Record_t::bytes);
      }
      ssd->clear();
    }
  } // if

  return true;
} // SortIterator::next
