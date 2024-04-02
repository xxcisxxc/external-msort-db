#include "Record.h"
#include "SortFunc.h"
#include "catch2/catch_amalgamated.hpp"
#include <cstdlib>

TEST_CASE("Test Creation of Records", "[record]") {
  Record<> *r = (Record<> *)malloc(sizeof(Record<>));
  r->key[0] = 1;
  REQUIRE(r->key[0] == 1);
  free(r);

  Record<int>::bytes = 2 * sizeof(char);
  Record<int> *r2 = (Record<int> *)malloc(Record<int>::bytes);
  r2->key[0] = 1;
  r2->key[1] = 2;
  REQUIRE(r2->key[0] == 1);
  REQUIRE(r2->key[1] == 2);
  free(r2);
}

TEST_CASE("Test Record Operators", "[record]") {
  Record_t::bytes = 3 * sizeof(char);
  Record_t *r1 = new Record_t;
  r1->key[0] = 1;
  r1->key[1] = 2;
  r1->key[2] = 3;
  Record_t *r2 = new Record_t;
  r2->key[0] = 1;
  r2->key[1] = 2;
  r2->key[2] = 4;
  REQUIRE(*r1 < *r2);
  REQUIRE(*r1 != *r2);
  REQUIRE(*r1 <= *r2);
  REQUIRE(*r1 <= *r1);
  REQUIRE(*r2 > *r1);
  REQUIRE(*r2 >= *r1);
  REQUIRE(*r2 >= *r2);
  REQUIRE(*r1 == *r1);

  Record_t::bytes = 2 * sizeof(char);
  REQUIRE(*r1 == *r2);
}

TEST_CASE("Test Record XOR", "[record]") {
  Record_t::bytes = 5 * sizeof(char);

  Record_t *r1 = new Record_t;
  r1->key[0] = 1;
  r1->key[1] = 2;
  r1->key[2] = 3;
  r1->key[3] = 4;
  r1->key[4] = 5;
  Record_t *r2 = new Record_t;
  r2->key[0] = 2;
  r2->key[1] = 3;
  r2->key[2] = 4;
  r2->key[3] = 5;
  r2->key[4] = 6;
  Record_t &r3 = r1->x_or(*r2);
  REQUIRE(r3.key[0] == 3);
  REQUIRE(r3.key[1] == 1);
  REQUIRE(r3.key[2] == 7);
  REQUIRE(r3.key[3] == 1);
  REQUIRE(r3.key[4] == 3);
  REQUIRE(&r3 == r1);

  delete r1;
  delete r2;
}

TEST_CASE("Create an array of Records", "[record]") {
  Record_t::bytes = 5 * sizeof(char);

  RecordArr_t r(2);
  r[0].key[0] = 1;
  r[0].key[1] = 2;
  r[0].key[2] = 3;
  r[0].key[3] = 4;
  r[0].key[4] = 5;
  r[1].key[0] = 2;
  r[1].key[1] = 3;
  r[1].key[2] = 4;
  r[1].key[3] = 5;
  r[1].key[4] = 6;
  Record_t &r3 = r[1].x_or(r[0]);
  REQUIRE(r3.key[0] == 3);
  REQUIRE(r3.key[1] == 1);
  REQUIRE(r3.key[2] == 7);
  REQUIRE(r3.key[3] == 1);
  REQUIRE(r3.key[4] == 3);
}

TEST_CASE("Record Array Iterator", "[record]") {
  Record_t::bytes = 5 * sizeof(char);

  RecordArr_t r(3);
  r[0].key[0] = 1;
  r[0].key[1] = 2;
  r[0].key[2] = 3;
  r[0].key[3] = 4;
  r[0].key[4] = 5;

  r[1].key[0] = 1;
  r[1].key[1] = 3;
  r[1].key[2] = 4;
  r[1].key[3] = 5;
  r[1].key[4] = 6;

  r[2].key[0] = 0;
  r[2].key[1] = 3;
  r[2].key[2] = 4;
  r[2].key[3] = 5;
  r[2].key[4] = 6;

  RecordArr_t::Iterator it = r.begin();
  REQUIRE(it->key[0] == 1);
  REQUIRE(it[0].key[1] == 2);
  it++;
  REQUIRE(it->key[0] == 1);
  REQUIRE(it[0].key[1] == 3);

  REQUIRE(it[1].key[0] == 0);

  REQUIRE(r.end() - r.begin() == 3);
}

TEST_CASE("Record Array Sort", "[record]") {
  Record_t::bytes = 5 * sizeof(char);

  RecordArr_t r(3);
  r[0].key[0] = 1;
  r[0].key[1] = 2;
  r[0].key[2] = 3;
  r[0].key[3] = 4;
  r[0].key[4] = 5;

  r[1].key[0] = 1;
  r[1].key[1] = 3;
  r[1].key[2] = 4;
  r[1].key[3] = 5;
  r[1].key[4] = 6;

  r[2].key[0] = 0;
  r[2].key[1] = 3;
  r[2].key[2] = 4;
  r[2].key[3] = 5;
  r[2].key[4] = 6;

  Index_t index(3);
  incache_sort(r, index, 3);

  REQUIRE(r[0].key[0] == 0);
  REQUIRE(r[0].key[1] == 3);
  REQUIRE(r[0].key[2] == 4);
  REQUIRE(r[0].key[3] == 5);
  REQUIRE(r[0].key[4] == 6);

  REQUIRE(r[1].key[0] == 1);
  REQUIRE(r[1].key[1] == 2);
  REQUIRE(r[1].key[2] == 3);
  REQUIRE(r[1].key[3] == 4);
  REQUIRE(r[1].key[4] == 5);

  REQUIRE(r[2].key[0] == 1);
  REQUIRE(r[2].key[1] == 3);
  REQUIRE(r[2].key[2] == 4);
  REQUIRE(r[2].key[3] == 5);
  REQUIRE(r[2].key[4] == 6);
}
