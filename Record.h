#pragma once

#include <cstdio>
#include <cstdlib>
#include <memory>
#include <new>
#include <stdexcept>
#include <cstring>

/**
 * @brief A Record is a key-value pair.
 *
 * A variable-sized Record whose size is determined by static member \p bytes .
 *
 * @tparam Key The type of the key.
 * @tparam Val The type of the value array.
 */
template <class Key = int, class Val = char> struct Record {
  static inline std::size_t bytes = sizeof(Key);

  Key key;
  Val val[];

  inline bool operator<(Record const &rhs) const { return key < rhs.key; }
  inline bool operator>(Record const &rhs) const { return key > rhs.key; }
  inline bool operator==(Record const &rhs) const { return key == rhs.key; }
  inline bool operator!=(Record const &rhs) const { return key != rhs.key; }
  inline bool operator<=(Record const &rhs) const { return key <= rhs.key; }
  inline bool operator>=(Record const &rhs) const { return key >= rhs.key; }

  inline Record &x_or(Record const &rhs) {
    if (bytes != rhs.bytes)
      throw std::invalid_argument("Record sizes do not match");
    key ^= rhs.key;
    for (std::size_t i = 0; i < (bytes - sizeof(Key)) / sizeof(Val); ++i)
      val[i] ^= rhs.val[i];
    return *this;
  }

  static inline void *operator new(std::size_t const size) {
    if (sizeof(Record) > bytes || size != sizeof(Record))
      throw std::bad_alloc();
    return malloc(bytes);
  }

  static inline void *operator new[](std::size_t const size) {
    if (size % sizeof(Record) != 0)
      throw std::bad_alloc();
    size_t const num_recs = size / sizeof(Record);
    return malloc(num_recs * bytes);
  }

  static inline void operator delete(void *const ptr) { free(ptr); }

  static inline void operator delete[](void *const ptr) { free(ptr); }

  static void copy_rec(Record source, Record *destination) {
    destination->key = source.key;
    std::memcpy(destination->val, source.val, bytes - sizeof(Key));
  }
};

/**
 * @brief Default Record type.
 *
 */
using Record_t = Record<>;

/**
 * @brief A RecordArr is a wrapper for Record Array.
 *
 *
 * @tparam Key The type of the key.
 * @tparam Val The type of the value array.
 */
template <class Key = int, class Val = char> struct RecordArr {
private:
  using shared_arr = std::shared_ptr<Record<Key, Val>>;
  shared_arr arr;

public:
  RecordArr() = default;
  RecordArr(std::size_t const size) : arr(new Record<Key, Val>[size]) {}
  ~RecordArr() = default;

  RecordArr(const shared_arr &recs) : arr(recs) {}

  RecordArr &operator=(const shared_arr &recs) {
    arr = recs;
    return *this;
  }

  Record<Key, Val> &operator[](std::size_t const idx) {
    return *reinterpret_cast<Record<Key, Val> *>(
        reinterpret_cast<char *>(arr.get()) + idx * Record<Key, Val>::bytes);
  }
  Record<Key, Val> const &operator[](std::size_t const idx) const {
    return *reinterpret_cast<Record<Key, Val> *>(
        reinterpret_cast<char *>(arr.get()) + idx * Record<Key, Val>::bytes);
  }
  // overload += operator
  RecordArr &operator+=(std::size_t const count) {
    arr = shared_arr(arr, reinterpret_cast<Record<Key, Val> *>(
                              reinterpret_cast<char *>(arr.get()) +
                              count * Record<Key, Val>::bytes));
    return *this;
  }
  Record<Key, Val> *data() { return arr.get(); }
};

/**
 * @brief Default RecordArr type.
 *
 */
using RecordArr_t = RecordArr<>;
