#pragma once

#include "Record.h"

typedef uint64_t RowCount;

class Plan {
  friend class Iterator;

public:
  Plan();
  virtual ~Plan();
  virtual class Iterator *init() const = 0;
  virtual RecordArr_t const &records() const = 0;
  virtual Record_t const &witnessRecord() const = 0;
  virtual RowCount const &getDuplicatesCount() const = 0;

private:
}; // class Plan

class Iterator {
public:
  Iterator();
  virtual ~Iterator();
  void run();
  virtual bool next() = 0;

private:
  RowCount _count;
}; // class Iterator
