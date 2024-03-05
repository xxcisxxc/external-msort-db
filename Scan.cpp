#include "Scan.h"
#include "Record.h"
#include "defs.h"
#include <climits>

ScanPlan::ScanPlan(RowCount const count) : _count(count), _records(count) {
  TRACE(true);
} // ScanPlan::ScanPlan

ScanPlan::~ScanPlan() { TRACE(true); } // ScanPlan::~ScanPlan

Iterator *ScanPlan::init() const {
  TRACE(true);
  return new ScanIterator(this);
} // ScanPlan::init

ScanIterator::ScanIterator(ScanPlan const *const plan)
    : _plan(plan), _count(0) {
  TRACE(true);
} // ScanIterator::ScanIterator

ScanIterator::~ScanIterator() {
  TRACE(true);
  traceprintf("produced %lu of %lu rows\n", (unsigned long)(_count),
              (unsigned long)(_plan->_count));
} // ScanIterator::~ScanIterator

void random_generate(Record_t &record) {
  record.key = std::rand();
  for (std::size_t i = 0; i < record.bytes; ++i)
    reinterpret_cast<char *>(record.val)[i] =
        std::rand() % (CHAR_MAX - CHAR_MIN + 1);
} // random_generate

bool ScanIterator::next() {
  TRACE(true);

  if (_count >= _plan->_count)
    return false;

  RecordArr_t records = _plan->_records;
  random_generate(records[_count]);

  traceprintf("produced %lu - %d: %d\n", (unsigned long)(_count),
              _plan->_records[_count].key, (int)_plan->_records[_count].val[0]);

  ++_count;
  return true;
} // ScanIterator::next
