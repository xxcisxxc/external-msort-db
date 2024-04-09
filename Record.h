#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <memory>
#include <new>
#include <stdexcept>
#include <string>
#include <utility>

/**
 * @brief A Record is a row with multiple columns.
 *
 * A variable-sized Record whose size is determined by static member \p bytes .
 *
 * @tparam Key The type of the key.
 * @tparam Val The type of the value array.
 */
template <class Key = char> struct Record {
  static inline std::size_t bytes = sizeof(Key);

  Key key[1];

  inline bool operator<(Record const &rhs) const {
    for (std::size_t i = 0; i < bytes / sizeof(Key); ++i) {
      if (key[i] < rhs.key[i])
        return true;
      else if (key[i] > rhs.key[i])
        return false;
    }
    return false;
  }
  inline bool operator>(Record const &rhs) const {
    for (std::size_t i = 0; i < bytes / sizeof(Key); ++i) {
      if (key[i] > rhs.key[i])
        return true;
      else if (key[i] < rhs.key[i])
        return false;
    }
    return false;
  }
  inline bool operator==(Record const &rhs) const {
    for (std::size_t i = 0; i < bytes / sizeof(Key); ++i) {
      if (key[i] != rhs.key[i])
        return false;
    }
    return true;
  }
  inline bool operator!=(Record const &rhs) const { return !(*this == rhs); }
  inline bool operator<=(Record const &rhs) const { return !(*this > rhs); }
  inline bool operator>=(Record const &rhs) const { return !(*this < rhs); }

  inline Record &x_or(Record const &rhs) {
    if (bytes != rhs.bytes)
      throw std::invalid_argument("Record sizes do not match");
    for (std::size_t i = 0; i < bytes / sizeof(Key); ++i)
      key[i] ^= rhs.key[i];
    return *this;
  }

  static inline void *operator new(std::size_t const size) {
    if (size != sizeof(Record))
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

  inline friend void swap(Record &lhs, Record &rhs) {
    for (std::size_t i = 0; i < bytes / sizeof(Key); ++i)
      std::swap(lhs.key[i], rhs.key[i]);
  }
  void swap(Record &rhs) { swap(*this, rhs); }
  inline Record &operator=(Record const &rhs) {
    for (std::size_t i = 0; i < bytes / sizeof(Key); ++i)
      key[i] = rhs.key[i];
    return *this;
  }

  static void copy_rec(const Record &source, Record *destination) {
    for (std::size_t i = 0; i < bytes / sizeof(Key); ++i) {
      destination->key[i] = source.key[i];
    }
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
template <class Key = char> struct RecordArr {
private:
  using shared_arr = std::shared_ptr<Record<Key>>;
  shared_arr arr;
  const std::size_t sz = 0;

public:
  struct Iterator {
    using iterator_category = std::random_access_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = Record<Key>;
    using pointer = Record<Key> *;
    using reference = Record<Key> &;
    Iterator(pointer ptr_) : ptr(ptr_) {}

    reference operator*() const { return *ptr; }
    pointer operator->() { return ptr; }
    Iterator &operator++() {
      ptr = reinterpret_cast<pointer>(reinterpret_cast<char *>(ptr) +
                                      value_type::bytes);
      return *this;
    }
    Iterator operator++(int) {
      Iterator tmp = *this;
      ++*this;
      return tmp;
    }
    Iterator &operator--() {
      ptr = reinterpret_cast<pointer>(reinterpret_cast<char *>(ptr) +
                                      value_type::bytes);
      return *this;
    }
    Iterator operator--(int) {
      Iterator tmp = *this;
      --*this;
      return tmp;
    }
    Iterator &operator+=(difference_type const n) {
      ptr = reinterpret_cast<pointer>(reinterpret_cast<char *>(ptr) +
                                      n * value_type::bytes);
      return *this;
    }
    Iterator operator+(difference_type const n) const {
      Iterator tmp = *this;
      return tmp += n;
    }
    Iterator &operator-=(difference_type const n) {
      ptr = reinterpret_cast<pointer>(reinterpret_cast<char *>(ptr) -
                                      n * value_type::bytes);
      return *this;
    }
    Iterator operator-(difference_type const n) const {
      Iterator tmp = *this;
      return tmp -= n;
    }
    difference_type operator-(Iterator const &rhs) const {
      return (reinterpret_cast<char *>(ptr) -
              reinterpret_cast<char *>(rhs.ptr)) /
             Record<Key>::bytes;
    }
    reference operator[](difference_type const n) const {
      return *reinterpret_cast<Record<Key> *>(reinterpret_cast<char *>(ptr) +
                                              n * Record<Key>::bytes);
    }
    bool operator==(Iterator const &rhs) const { return ptr == rhs.ptr; }
    bool operator!=(Iterator const &rhs) const { return ptr != rhs.ptr; }
    bool operator<(Iterator const &rhs) const { return ptr < rhs.ptr; }
    bool operator>(Iterator const &rhs) const { return ptr > rhs.ptr; }
    bool operator<=(Iterator const &rhs) const { return ptr <= rhs.ptr; }
    bool operator>=(Iterator const &rhs) const { return ptr >= rhs.ptr; }

  private:
    pointer ptr;
  };

  RecordArr() = default;
  RecordArr(std::size_t const size) : arr(new Record<Key>[size]), sz(size) {}
  RecordArr(shared_arr const &arr_, std::size_t const size)
      : arr(arr_), sz(size) {}
  ~RecordArr() = default;

  Record<Key> &operator[](std::size_t const idx) {
    if (idx >= sz)
      throw std::out_of_range("RecordArr index " + std::to_string(idx) +
                              " out of range " + std::to_string(sz));
    return *reinterpret_cast<Record<Key> *>(
        reinterpret_cast<char *>(arr.get()) + idx * Record<Key>::bytes);
  }
  Record<Key> const &operator[](std::size_t const idx) const {
    if (idx >= sz)
      throw std::out_of_range("RecordArr index " + std::to_string(idx) +
                              " out of range " + std::to_string(sz));
    return *reinterpret_cast<Record<Key> *>(
        reinterpret_cast<char *>(arr.get()) + idx * Record<Key>::bytes);
  }
  Iterator begin() { return Iterator(arr.get()); }
  Iterator end() {
    return Iterator(reinterpret_cast<Record<Key> *>(
        reinterpret_cast<char *>(arr.get()) + sz * Record<Key>::bytes));
  }
  Record<Key> *data() { return arr.get(); }
  shared_arr ptr() const { return arr; }
  shared_arr ptr(std::size_t offset_byte) const {
    char *offset_ptr = reinterpret_cast<char *>(arr.get()) + offset_byte;
    return shared_arr(arr, reinterpret_cast<Record<Key> *>(offset_ptr));
  }
  std::size_t size() const { return sz; }
  RecordArr &operator+=(std::size_t const count) {
    if (count > sz)
      throw std::out_of_range("RecordArr count " + std::to_string(count) +
                              " out of range " + std::to_string(sz));
    arr = shared_arr(arr, reinterpret_cast<Record<Key> *>(
                              reinterpret_cast<char *>(arr.get()) +
                              count * Record<Key>::bytes));
    const_cast<std::size_t &>(sz) -= count;
    return *this;
  }
  RecordArr operator+(std::size_t const count) const {
    if (count > sz)
      throw std::out_of_range("RecordArr count " + std::to_string(count) +
                              " out of range " + std::to_string(sz));
    shared_arr new_arr = shared_arr(
        arr,
        reinterpret_cast<Record<Key> *>(reinterpret_cast<char *>(arr.get()) +
                                        count * Record<Key>::bytes));
    return RecordArr(new_arr, sz - count);
  }
};

/**
 * @brief Default RecordArr type.
 *
 */
using RecordArr_t = RecordArr<>;

/**
 * @brief Index of a Record Array.
 *
 */
template <class Ind = uint16_t> struct Index {
private:
  using shared_arr = std::shared_ptr<Ind>;
  shared_arr arr;
  const std::size_t sz = 0;

public:
  struct Iterator {
    using iterator_category = std::random_access_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = Ind;
    using pointer = Ind *;
    using reference = Ind &;
    Iterator(pointer ptr_) : ptr(ptr_) {}

    reference operator*() const { return *ptr; }
    pointer operator->() { return ptr; }
    Iterator &operator++() {
      ++ptr;
      return *this;
    }
    Iterator operator++(int) {
      Iterator tmp = *this;
      ++*this;
      return tmp;
    }
    Iterator &operator--() {
      --ptr;
      return *this;
    }
    Iterator operator--(int) {
      Iterator tmp = *this;
      --*this;
      return tmp;
    }
    Iterator &operator+=(difference_type const n) {
      ptr += n;
      return *this;
    }
    Iterator operator+(difference_type const n) const {
      Iterator tmp = *this;
      return tmp += n;
    }
    Iterator &operator-=(difference_type const n) {
      ptr -= n;
      return *this;
    }
    Iterator operator-(difference_type const n) const {
      Iterator tmp = *this;
      return tmp -= n;
    }
    difference_type operator-(Iterator const &rhs) const {
      return ptr - rhs.ptr;
    }
    reference operator[](difference_type const n) const { return *(ptr + n); }
    bool operator==(Iterator const &rhs) const { return ptr == rhs.ptr; }
    bool operator!=(Iterator const &rhs) const { return ptr != rhs.ptr; }
    bool operator<(Iterator const &rhs) const { return ptr < rhs.ptr; }
    bool operator>(Iterator const &rhs) const { return ptr > rhs.ptr; }
    bool operator<=(Iterator const &rhs) const { return ptr <= rhs.ptr; }
    bool operator>=(Iterator const &rhs) const { return ptr >= rhs.ptr; }

  private:
    pointer ptr;
  };

  Index() = default;
  Index(std::size_t const size) : arr(new Ind[size]), sz(size) {}
  Index(shared_arr const &arr_, std::size_t const size) : arr(arr_), sz(size) {}
  ~Index() = default;

  Ind &operator[](std::size_t const idx) {
    if (idx >= sz)
      throw std::out_of_range("RecordArr index out of range");
    return *(arr.get() + idx);
  }
  Ind const &operator[](std::size_t const idx) const {
    if (idx >= sz)
      throw std::out_of_range("RecordArr index out of range");
    return *(arr.get() + idx);
  }
  Iterator begin() { return Iterator(arr.get()); }
  Iterator end() { return Iterator(arr.get() + sz); }
  Ind *data() { return arr.get(); }
  std::size_t size() const { return sz; }
};

using Index_t = Index<>;

struct MergeInd {
  uint32_t run_id;
  uint32_t record_id;
};
using Index_r = Index<MergeInd>;
