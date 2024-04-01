#include "Iterator.h"
#include "Record.h"

class ScanPlan : public Plan {
  friend class ScanIterator;

public:
  ScanPlan(RowCount const count);
  ~ScanPlan();
  Iterator *init() const override;
  RecordArr_t const &records() const override { return _records; }
  Record_t *witnessRecord = new Record_t;

private:
  RowCount const _count;
  RecordArr_t const _records;
}; // class ScanPlan

class ScanIterator : public Iterator {
public:
  ScanIterator(ScanPlan const *const plan);
  ~ScanIterator();
  bool next();

private:
  ScanPlan const *const _plan;
  RowCount _count;
}; // class ScanIterator
