#include "Scan.h"
#include "Consts.h"
#include "Iterator.h"
#include "Record.h"
#include "Utils.h"
#include "defs.h"
#include <ctime>

ScanPlan::ScanPlan(RowCount const count)
    : _count(count),
      _rcache(std::shared_ptr<Record_t>(
                  reinterpret_cast<Record_t *>(new char[kCacheSize])),
              fcache_nrecords()),
      _inputWitnessRecord(new Record_t) {
  TRACE(true);
  _inputWitnessRecord->fill(0);
} // ScanPlan::ScanPlan

ScanPlan::~ScanPlan() { TRACE(true); } // ScanPlan::~ScanPlan

Iterator *ScanPlan::init() const {
  TRACE(true);
  return new ScanIterator(this);
} // ScanPlan::init

ScanIterator::ScanIterator(ScanPlan const *const plan)
    : _plan(plan), _count(0), _kRowCache(cache_nrecords()) {
  TRACE(true);
} // ScanIterator::ScanIterator

ScanIterator::~ScanIterator() {
  TRACE(true);
  traceprintf("produced %lu of %lu rows\n", (unsigned long)(_count),
              (unsigned long)(_plan->_count));
} // ScanIterator::~ScanIterator

/**
 * @brief Fast Random Generator
 * @ref https://prng.di.unimi.it
 *
 */

static inline uint64_t rotl(const uint64_t x, int k) {
  return (x << k) | (x >> (64 - k));
}

uint64_t prng_seed() {
  static uint64_t x = time(NULL);

  uint64_t z = (x += 0x9e3779b97f4a7c15);
  z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
  z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
  return z ^ (z >> 31);
}

uint64_t prng_rand(void) {
  static uint64_t s[4] = {prng_seed(), prng_seed(), prng_seed(), prng_seed()};

  const uint64_t result = rotl(s[0] + s[3], 23) + s[0];

  const uint64_t t = s[1] << 17;

  s[2] ^= s[0];
  s[3] ^= s[1];
  s[1] ^= s[2];
  s[0] ^= s[3];

  s[2] ^= t;

  s[3] = rotl(s[3], 45);

  return result;
}

union random_t {
  uint64_t i;
  struct {
    uint8_t key[6];
    // 3 bit: larger than 5, use current duplicate
    // 3 bit: larger than 5, generate new duplicate
    // every two bits: 01: number, 10: upper case, 11 / 00: lower case
    uint16_t alpha_num;
  } s;
  struct {
    int32_t use_dup;
    uint8_t coeff[2];
    uint16_t gen_dup;
  } d;
};

void gen_record(Record_t &rec) {
  for (std::size_t i = 0; i < rec.bytes; i += sizeof(random_t::s.key)) {
    uint8_t range = std::min(sizeof(random_t::s.key), rec.bytes - i);
    random_t r = {.i = prng_rand()};
    for (std::size_t j = 0; j < range; ++j) {
      if ((r.s.alpha_num & 0b11) == 0b01) {
        rec.key[i + j] = r.s.key[j] % 10 + '0';
      } else if ((r.s.alpha_num & 0b11) == 0b10) {
        rec.key[i + j] = r.s.key[j] % 26 + 'A';
      } else {
        rec.key[i + j] = r.s.key[j] % 26 + 'a';
      }
      r.s.alpha_num >>= 2;
    }
  }
}

void random_generate(Record_t &record) {
  static Record_t *dup = nullptr;
  if (dup == nullptr) {
    dup = new Record_t;
    gen_record(*dup);
  }

  random_t r = {.i = prng_rand()};
  if (r.d.use_dup > 0) {
    if (r.d.use_dup * r.d.coeff[0] + r.d.coeff[1] < r.d.gen_dup) {
      gen_record(*dup);
    }
    record = *dup;
  } else {
    gen_record(record);
  }
} // random_generate

bool ScanIterator::next() {
  TRACE(true);

  RecordArr_t records = _plan->_rcache;

  if (_count >= _plan->_count) {
    return false;
  }

  random_generate(records[_count % _kRowCache]);

  // witness for input
  (*(_plan->_inputWitnessRecord.get())).x_or(records[_count % _kRowCache]);

  // traceprintf("produced %lu - %d %d\n", (unsigned long)(_count),
  //             _plan->_rcache[_count % _kRowCache].key[0],
  //             _plan->_rcache[_count % _kRowCache].key[1]);

  ++_count;
  return true;
} // ScanIterator::next
