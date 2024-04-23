#pragma once

#include "Consts.h"
#include "Record.h"
#include <cstddef>

static inline std::size_t cache_nruns() { return 1; } // cache_nruns

static inline std::size_t cache_nrecords() {
  // std::size_t n_records = kCacheSize / (Record_t::bytes + sizeof(uint16_t));
  // return n_records - n_records % 2;
  return 8;
} // cache_nrecords

template <typename From, typename To>
static inline std::shared_ptr<To> ptr_cast(std::shared_ptr<From> ptr) {
  return std::shared_ptr<To>(ptr, reinterpret_cast<To *>(ptr.get()));
} // ptr_cast

static inline std::size_t mem_nruns() { return 4; } // mem_nruns

static inline std::size_t mem_nrecords() {
  // return kMemSize / (Record_t::bytes);
  return cache_nrecords() * mem_nruns();
}

static inline std::size_t ssd_nrecords() {
  // return kSSDSize / (Record_t::bytes);
  return 256;
}
