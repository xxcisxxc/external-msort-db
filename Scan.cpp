#include "Scan.h"
#include "Consts.h"
#include "Record.h"
#include "Utils.h"
#include "defs.h"
#include <climits>

ScanPlan::ScanPlan(RowCount const count)
    : _count(count),
      _rcache(std::shared_ptr<Record_t>(
                  reinterpret_cast<Record_t *>(new char[kCacheSize])),
              kCacheSize / Record_t::bytes),
      _inputWitnessRecord(new Record_t) {
  TRACE(true);
  _inputWitnessRecord->fill(0);
} // ScanPlan::ScanPlan

ScanPlan::~ScanPlan() { TRACE(true); } // ScanPlan::~ScanPlan

Iterator *ScanPlan::init() const {
  TRACE(true);
  return new ScanIterator(this);
} // ScanPlan::init

ScanIterator::ScanIterator(ScanPlan const *const plan)
    : _plan(plan), _count(0), _kRowCache(cache_nrecords()) {
  TRACE(true);
} // ScanIterator::ScanIterator

ScanIterator::~ScanIterator() {
  TRACE(true);
  traceprintf("produced %lu of %lu rows\n", (unsigned long)(_count),
              (unsigned long)(_plan->_count));
} // ScanIterator::~ScanIterator

void random_generate(Record_t &record) {
  for (std::size_t i = 0; i < record.bytes; ++i)
    reinterpret_cast<char *>(record.key)[i] =
        std::rand() % (CHAR_MAX - CHAR_MIN + 1);
} // random_generate

bool ScanIterator::next() {
  TRACE(true);

  if (_count >= _plan->_count)
    return false;

  RecordArr_t records = _plan->_rcache;
  random_generate(records[_count % _kRowCache]);

  // witness for input
  (*(_plan->_inputWitnessRecord.get())).x_or(records[_count % _kRowCache]);

  traceprintf("produced %lu - %d %d\n", (unsigned long)(_count),
              _plan->_rcache[_count % _kRowCache].key[0],
              _plan->_rcache[_count % _kRowCache].key[1]);

  ++_count;
  return true;
} // ScanIterator::next
