#include "Record.h"
#include "SortFunc.h"
#include "catch2/catch_amalgamated.hpp"

TEST_CASE("InMemMerge", "[sortfunc]") {
  Record_t::bytes = sizeof(char);

  RecordArr_t r(4 * 8);
  // run 1
  r[0].key[0] = 1;
  r[1].key[0] = 2;
  r[2].key[0] = 6;
  r[3].key[0] = 7;
  r[4].key[0] = 10;
  r[5].key[0] = 14;
  r[6].key[0] = 22;
  r[7].key[0] = 28;

  // run 2
  r[8].key[0] = -8;
  r[9].key[0] = -7;
  r[10].key[0] = -6;
  r[11].key[0] = -5;
  r[12].key[0] = -4;
  r[13].key[0] = -3;
  r[14].key[0] = -2;
  r[15].key[0] = -1;

  // run 3
  r[16].key[0] = 0;
  r[17].key[0] = 3;
  r[18].key[0] = 4;
  r[19].key[0] = 5;
  r[20].key[0] = 6;
  r[21].key[0] = 7;
  r[22].key[0] = 8;
  r[23].key[0] = 9;

  // run 4
  r[24].key[0] = 31;
  r[25].key[0] = 32;
  r[26].key[0] = 33;
  r[27].key[0] = 34;
  r[28].key[0] = 35;
  r[29].key[0] = 36;
  r[30].key[0] = 37;
  r[31].key[0] = 38;

  Index_t index(3);
  incache_sort(r, index, 3);

  RecordArr_t out(4);
  Device ssd("tests/ssd", 1, 1, 1);
  Index_r index_r(4);
  inmem_merge(r, {4, out}, &ssd, index_r, {8, 4}, 0);

  ssd.eread(reinterpret_cast<char *>(r.data()), 4 * 8 * Record_t::bytes, 0);

  REQUIRE(r[0].key[0] == -8);
  REQUIRE(r[1].key[0] == -7);
  REQUIRE(r[2].key[0] == -6);
  REQUIRE(r[3].key[0] == -5);
  REQUIRE(r[4].key[0] == -4);
  REQUIRE(r[5].key[0] == -3);
  REQUIRE(r[6].key[0] == -2);
  REQUIRE(r[7].key[0] == -1);
  REQUIRE(r[8].key[0] == 0);
  REQUIRE(r[9].key[0] == 1);
  REQUIRE(r[10].key[0] == 2);
  REQUIRE(r[11].key[0] == 3);
  REQUIRE(r[12].key[0] == 4);
  REQUIRE(r[13].key[0] == 5);
  REQUIRE(r[14].key[0] == 6);
  REQUIRE(r[15].key[0] == 6);
  REQUIRE(r[16].key[0] == 7);
  REQUIRE(r[17].key[0] == 7);
  REQUIRE(r[18].key[0] == 8);
  REQUIRE(r[19].key[0] == 9);
  REQUIRE(r[20].key[0] == 10);
  REQUIRE(r[21].key[0] == 14);
  REQUIRE(r[22].key[0] == 22);
  REQUIRE(r[23].key[0] == 28);
  REQUIRE(r[24].key[0] == 31);
  REQUIRE(r[25].key[0] == 32);
  REQUIRE(r[26].key[0] == 33);
  REQUIRE(r[27].key[0] == 34);
  REQUIRE(r[28].key[0] == 35);
  REQUIRE(r[29].key[0] == 36);
  REQUIRE(r[30].key[0] == 37);
  REQUIRE(r[31].key[0] == 38);
}
