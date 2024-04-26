#pragma once

#include <cstddef>
#include <cstdint>

constexpr std::size_t kCacheSize = 1024 * 1024;           //!< 1MB cache size
constexpr std::size_t kMemSize = 100 * 1024 * 1024;       //!< 100MB memory size
constexpr uint64_t kSSDSize = 10ULL * 1024 * 1024 * 1024; //!< 10GB SSD size

constexpr char const *kSSD = "ssd";
constexpr char const *kHDD = "hdd";
constexpr char const *kOut = "hddout";
constexpr char const *kDupOut = "dupout";
