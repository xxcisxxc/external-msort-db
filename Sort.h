#include "Iterator.h"
#include "Record.h"

class SortPlan : public Plan {
  friend class SortIterator;

public:
  SortPlan(Plan *const input);
  ~SortPlan();
  Iterator *init() const override;
  inline RecordArr_t const &records() const override {
    return _input->records();
  }

private:
  Plan *const _input;
  RecordArr_t const &_records;
}; // class SortPlan

class SortIterator : public Iterator {
public:
  SortIterator(SortPlan const *const plan);
  ~SortIterator();
  bool next();

private:
  SortPlan const *const _plan;
  Iterator *const _input;
  RowCount _consumed, _produced;
}; // class SortIterator
