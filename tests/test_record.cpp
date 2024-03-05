#include "Record.h"
#include "catch2/catch_amalgamated.hpp"
#include <cstdlib>
#include <memory>

TEST_CASE("Test Creation of Records", "[record]") {
  Record<> *r = (Record<> *)malloc(sizeof(Record<>));
  r->key = 1;
  REQUIRE(r->key == 1);
  free(r);

  Record<int, int>::bytes = sizeof(int) + sizeof(int);
  Record<int, int> *r2 = (Record<int, int> *)malloc(Record<int, int>::bytes);
  r2->key = 1;
  r2->val[0] = 2;
  REQUIRE(r2->key == 1);
  REQUIRE(r2->val[0] == 2);
  free(r2);

  RecordArr_t r3 = std::make_shared<Record_t>();
  r3[0].key = 4;
  REQUIRE(r3[0].key == 4);

  RecordArr_t r4 = std::shared_ptr<Record_t>(new Record_t[10]);
  r4[0].key = 5;
  REQUIRE(r4[0].key == 5);
  r4[1].key = 6;
  REQUIRE(r4[1].key == 6);
}

TEST_CASE("Test Record Operators", "[record]") {
  Record_t r1;
  r1.key = 1;
  Record_t r2;
  r2.key = 2;
  REQUIRE(r1 < r2);
  REQUIRE(r1 != r2);
  REQUIRE(r1 <= r2);
  REQUIRE(r1 <= r1);
  REQUIRE(r2 > r1);
  REQUIRE(r2 >= r1);
  REQUIRE(r2 >= r2);
  REQUIRE(r1 == r1);
}

TEST_CASE("Test Record XOR", "[record]") {
  Record_t::bytes = sizeof(int) + sizeof(int);

  Record_t *r1 = new Record_t;
  r1->key = 1;
  r1->val[0] = 2;
  r1->val[1] = 3;
  r1->val[2] = 4;
  r1->val[3] = 5;
  Record_t *r2 = new Record_t;
  r2->key = 2;
  r2->val[0] = 3;
  r2->val[1] = 4;
  r2->val[2] = 5;
  r2->val[3] = 6;
  Record_t &r3 = r1->x_or(*r2);
  REQUIRE(r3.key == 3);
  REQUIRE(r3.val[0] == 1);
  REQUIRE(r3.val[1] == 7);
  REQUIRE(r3.val[2] == 1);
  REQUIRE(r3.val[3] == 3);
  REQUIRE(&r3 == r1);

  delete r1;
  delete r2;
}

TEST_CASE("Create an array of Records", "[record]") {
  Record_t::bytes = sizeof(int) + sizeof(int);

  RecordArr_t r(10);
  r[0].key = 1;
  r[0].val[0] = 2;
  r[0].val[1] = 3;
  r[0].val[2] = 4;
  r[0].val[3] = 5;
  r[1].key = 2;
  r[1].val[0] = 3;
  r[1].val[1] = 4;
  r[1].val[2] = 5;
  r[1].val[3] = 6;
  Record_t &r3 = r[1].x_or(r[0]);
  REQUIRE(r3.key == 3);
  REQUIRE(r3.val[0] == 1);
  REQUIRE(r3.val[1] == 7);
  REQUIRE(r3.val[2] == 1);
  REQUIRE(r3.val[3] == 3);
}
