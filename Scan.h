#include "Iterator.h"
#include "Record.h"

class ScanPlan : public Plan {
  friend class ScanIterator;

public:
  ScanPlan(RowCount const count);
  ~ScanPlan();
  Iterator *init() const override;
  inline RecordArr_t const &records() const override { return _rcache; }
  Record_t *inputWitnessRecord = new Record_t();
  virtual Record_t *witnessRecord() const override {
    return inputWitnessRecord;
  }

private:
  RowCount const _count;
  // Cache-resident records
  RecordArr_t const _rcache;
}; // class ScanPlan

class ScanIterator : public Iterator {
public:
  ScanIterator(ScanPlan const *const plan);
  ~ScanIterator();
  bool next();

private:
  ScanPlan const *const _plan;
  RowCount _count;
  // max number of rows in cache
  const RowCount _kRowCache;
}; // class ScanIterator
