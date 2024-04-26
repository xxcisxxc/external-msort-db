#pragma once

#include "Consts.h"
#include "Record.h"
#include <cstddef>

static inline std::size_t cache_nruns() { return 1; } // cache_nruns

static inline std::size_t fcache_nrecords() {
  return kCacheSize / Record_t::bytes;
} // fcache_nrecords

static inline std::size_t cache_nrecords() {
  return 8;
  std::size_t n_records = kCacheSize / (Record_t::bytes + sizeof(uint16_t));
  return n_records - n_records % 2;
} // cache_nrecords

template <typename From, typename To>
static inline std::shared_ptr<To> ptr_cast(std::shared_ptr<From> ptr) {
  return std::shared_ptr<To>(ptr, reinterpret_cast<To *>(ptr.get()));
} // ptr_cast

static inline std::size_t fmem_nrecords() {
  return kMemSize / Record_t::bytes;
} // fmem_nrecords

static inline std::size_t out_nrecords() {
  return 16;
  return kCacheSize / Record_t::bytes;
} // out_nrecords

static inline std::size_t mmem_nrecords() {
  return 32;
  return (kMemSize - kCacheSize) / Record_t::bytes;
} // mmem_nrecords

static inline std::size_t mem_nruns() {
  return 4;
  return mmem_nrecords() / cache_nrecords();
} // mem_nruns

static inline std::size_t mem_nrecords() {
  return cache_nrecords() * mem_nruns();
} // mem_nrecords

static inline std::size_t fssd_nrecords() {
  return kSSDSize / Record_t::bytes;
} // fssd_nrecords

static inline std::size_t ssd_nruns() {
  return 8;
  return fssd_nrecords() / mem_nrecords();
} // ssd_nruns

static inline std::size_t ssd_nrecords() {
  return mem_nrecords() * ssd_nruns();
} // ssd_nrecords

static inline std::size_t minm_nrecords() {
  return 4;
  constexpr std::size_t min_size = 100 * 5 * 1024 * 1024 / 1000;
  return min_size / Record_t::bytes;
} // minm_nrecords
