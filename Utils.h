#pragma once

#include "Consts.h"
#include "Record.h"
#include <cstddef>


static inline std::size_t cache_nrecords() {
  return kCacheSize / (Record_t::bytes + sizeof(uint16_t));
} // cache_nrecords

template <typename From, typename To>
static inline std::shared_ptr<To> ptr_cast(std::shared_ptr<From> ptr) {
  return std::shared_ptr<To>(ptr, reinterpret_cast<To *>(ptr.get()));
} // ptr_cast

// TODO::ranjitha - confirm if this is right
static inline std::size_t dram_nrecords() {
  //return kMemSize / (Record_t::bytes);
  return 32;
} 

static inline std::size_t cache_run_nrecords() {
  //return kMemSize / (Record_t::bytes);
  return 8;
} 

static inline std::size_t ssd_nrecords() {
  //return kSSDSize / (Record_t::bytes);
  return 256;
} 

