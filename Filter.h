#include "Iterator.h"
#include "Record.h"

class FilterPlan : public Plan {
  friend class FilterIterator;

public:
  FilterPlan(Plan *const input);
  ~FilterPlan();
  Iterator *init() const override;
  inline RecordArr_t const &records() const override { return _records; }

private:
  Plan *const _input;
  /* One Run */
  RecordArr_t const &_records;
}; // class FilterPlan

class FilterIterator : public Iterator {
public:
  FilterIterator(FilterPlan const *const plan);
  ~FilterIterator();
  bool next();

private:
  FilterPlan const *const _plan;
  Iterator *const _input;
  RowCount _consumed, _produced;
}; // class FilterIterator
