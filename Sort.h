#include "Consts.h"
#include "Device.h"
#include "Iterator.h"
#include "Record.h"
#include "Utils.h"
#include <cstdint>
#include <memory>

class SortPlan : public Plan {
  friend class SortIterator;

public:
  SortPlan(Plan *const input);
  ~SortPlan();
  Iterator *init() const override;
  Record_t const &witnessRecord() const override { return _inputWitnessRecord; }
  RecordArr_t const &records() const override { return _rmem.out; }

private:
  struct CacheRun {
    RecordArr_t records;
    Index_t index;
    CacheRun(RecordArr_t const &records)
        : records(records.ptr(), cache_nrecords()),
          index(ptr_cast<Record_t, uint16_t>(
                    records.ptr(cache_nrecords() * Record_t::bytes)),
                cache_nrecords()) {}
  }; // struct CacheRun
  struct CacheInd {
    Index_r index;
    CacheInd(RecordArr_t const &records)
        : index(ptr_cast<Record_t, MergeInd>(records.ptr()),
                kCacheSize / sizeof(MergeInd)) {}
  }; // struct CacheInd
  struct MemRun {
    RecordArr_t out;
    RecordArr_t work;
    MemRun(RecordArr_t const &records)
        : out(records.ptr(), kCacheSize / Record_t::bytes),
          work(records.ptr(kCacheSize),
               (kMemSize - kCacheSize) / Record_t::bytes) {}
    RecordArr_t whole() const {
      return RecordArr_t(out.ptr(), out.size() + work.size());
    }
  }; // struct MemRun
  Plan *const _input;
  CacheRun _rcache;
  CacheInd _icache;
  MemRun _rmem;

  std::unique_ptr<Device> ssd;
  std::unique_ptr<Device> hdd;
  std::unique_ptr<Device> hddout;

  Record_t const &_inputWitnessRecord;
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
  RowCount const _kRowCacheRun;
  RowCount const _kRowMemRun;
  RowCount const _kRowMemOut;
}; // class SortIterator
