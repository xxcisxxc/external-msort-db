#include "Device.h"
#include "Iterator.h"
#include "Record.h"

class ValidatePlan : public Plan {
  friend class ValidateIterator;

public:
  ValidatePlan(Plan *const input);
  ~ValidatePlan();
  Iterator *init() const override;

  Record_t const &witnessRecord() const override {
    return *(_outputWitnessRecord.get());
  }
  RecordArr_t const &records() const override { return _buffer; }

private:
  Plan *const _input;
  std::unique_ptr<Record_t> const _outputWitnessRecord;
  Record_t const &_inputWitnessRecord;

  RecordArr_t const _buffer;
}; // class ScanPlan

class ValidateIterator : public Iterator {
public:
  ValidateIterator(ValidatePlan const *const plan);
  ~ValidateIterator();
  bool next();

private:
  RowCount _count;

  ValidatePlan const *const _plan;
  Iterator *const _input;
  ReadDevice _out;
}; // class ScanIterator
