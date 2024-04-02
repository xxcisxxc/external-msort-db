#include "Sort.h"
#include "Record.h"
#include "SortFunc.h"
#include "defs.h"

SortPlan::SortPlan(Plan *const input)
    : _input(input), _rcache(input->records()), _icache(input->records()),
      _rmem(RecordArr_t(std::shared_ptr<Record_t>(
                            reinterpret_cast<Record_t *>(new char[kMemSize])),
                        kMemSize / Record_t::bytes)) {
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
      _kRowCache(_plan->_rcache.records.size()) {
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

  RecordArr_t records = _plan->_rcache.records;
  records += _consumed;
  Index_t indext = _plan->_rcache.index;

  do {
    if (!_input->next()) {
      break;
    }
    ++_consumed;
  } while (_consumed % 4 != 0);

  if (_produced >= _consumed) {
    return false;
  }

  RecordArr_t work = _plan->_rmem.work + _produced;
  incache_sort(records, work, indext, _consumed - _produced);
  _produced = _consumed;

  if (_consumed % 8 == 0) {
    Index_r indexr = _plan->_icache.index;
    RecordArr_t in = _plan->_rmem.work;
    RecordArr_t out = _plan->_rmem.out;
    inmem_merge(in, out, indexr, 4, 2);
  }

  return true;
} // SortIterator::next
