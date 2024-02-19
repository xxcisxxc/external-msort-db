#include "Sort.h"
#include "Record.h"

SortPlan::SortPlan(Plan *const input)
    : _input(input), _records(input->records()) {
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
    : _plan(plan), _input(plan->_input->init()), _consumed(0), _produced(0) {
  TRACE(true);

  while (_input->next())
    ++_consumed;
  delete _input;

  traceprintf("consumed %lu rows\n", (unsigned long)(_consumed));
} // SortIterator::SortIterator

SortIterator::~SortIterator() {
  TRACE(true);

  traceprintf("produced %lu of %lu rows\n", (unsigned long)(_produced),
              (unsigned long)(_consumed));
} // SortIterator::~SortIterator

void simple_sort(RecordArr_t &records, std::size_t const size) {
  std::qsort(records.data(), size, Record_t::bytes,
             [](void const *a, void const *b) {
               return reinterpret_cast<Record_t const *>(a)->key -
                      reinterpret_cast<Record_t const *>(b)->key;
             });
} // simple_merge_sort

bool SortIterator::next() {
  TRACE(true);

  RecordArr_t records = _plan->_records;
  if (_produced >= _consumed) {
    simple_sort(records, _consumed * kCacheRunSize);
    return false;
  }

  ++_produced;
  return true;
} // SortIterator::next
