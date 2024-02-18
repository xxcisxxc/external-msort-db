#pragma once

#include <array>
#include <bitset>

template <class Key = int, class Val = char, std::size_t Size = sizeof(Key)>
struct Record {
  static_assert(Size - sizeof(Key) >= 0);

  union {
    struct {
      Key key;
      std::array<Val, Size - sizeof(Key)> val;
    };
    std::bitset<Size * 8> bits;
  };

  inline bool operator<(Record const &rhs) const { return key < rhs.key; }
  inline bool operator>(Record const &rhs) const { return key > rhs.key; }
  inline bool operator==(Record const &rhs) const { return key == rhs.key; }
  inline bool operator!=(Record const &rhs) const { return key != rhs.key; }
  inline bool operator<=(Record const &rhs) const { return key <= rhs.key; }
  inline bool operator>=(Record const &rhs) const { return key >= rhs.key; }

  inline Record operator^(Record const &rhs) const {
    Record result;
    result.bits = bits ^ rhs.bits;
    return result;
  }
};
