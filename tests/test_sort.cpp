#include "Device.h"
#include "Record.h"
#include "SortFunc.h"
#include "catch2/catch_amalgamated.hpp"

TEST_CASE("InMemMerge", "[sortfunc]") {
  Record_t::bytes = sizeof(char);

  RecordArr_t r(6 * 8);
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

  // run 5
  r[32].key[0] = 12;
  r[33].key[0] = 13;
  r[34].key[0] = 16;
  r[35].key[0] = 17;
  r[36].key[0] = 20;
  r[37].key[0] = 54;
  r[38].key[0] = 62;
  r[39].key[0] = 78;

  // run 6
  r[40].key[0] = 11;
  r[41].key[0] = 15;
  r[42].key[0] = 18;
  r[43].key[0] = 19;
  r[44].key[0] = 21;
  r[45].key[0] = 23;
  r[46].key[0] = 24;
  r[47].key[0] = 25;

  RecordArr_t out(4);
  Device ssd("tests/ssd", 1, 1, 1);

  Index_r index_r(8);
  inmem_merge(r, {4, out}, &ssd, index_r, {8, 6});

  ssd.eread(reinterpret_cast<char *>(r.data()), 6 * 8 * Record_t::bytes, 0);

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
  REQUIRE(r[21].key[0] == 11);
  REQUIRE(r[22].key[0] == 12);
  REQUIRE(r[23].key[0] == 13);
  REQUIRE(r[24].key[0] == 14);
  REQUIRE(r[25].key[0] == 15);
  REQUIRE(r[26].key[0] == 16);
  REQUIRE(r[27].key[0] == 17);
  REQUIRE(r[28].key[0] == 18);
  REQUIRE(r[29].key[0] == 19);
  REQUIRE(r[30].key[0] == 20);
  REQUIRE(r[31].key[0] == 21);
  REQUIRE(r[32].key[0] == 22);
  REQUIRE(r[33].key[0] == 23);
  REQUIRE(r[34].key[0] == 24);
  REQUIRE(r[35].key[0] == 25);
  REQUIRE(r[36].key[0] == 28);
  REQUIRE(r[37].key[0] == 31);
  REQUIRE(r[38].key[0] == 32);
  REQUIRE(r[39].key[0] == 33);
  REQUIRE(r[40].key[0] == 34);
  REQUIRE(r[41].key[0] == 35);
  REQUIRE(r[42].key[0] == 36);
  REQUIRE(r[43].key[0] == 37);
  REQUIRE(r[44].key[0] == 38);
  REQUIRE(r[45].key[0] == 54);
  REQUIRE(r[46].key[0] == 62);
  REQUIRE(r[47].key[0] == 78);
}

TEST_CASE("SpillMemMerge", "[sortfunc]") {
  Record_t::bytes = sizeof(char);

  RecordArr_t r(6 * 8);
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

  // run 5
  r[32].key[0] = 12;
  r[33].key[0] = 13;
  r[34].key[0] = 16;
  r[35].key[0] = 17;
  r[36].key[0] = 20;
  r[37].key[0] = 54;
  r[38].key[0] = 62;
  r[39].key[0] = 78;

  // run 6
  r[40].key[0] = 11;
  r[41].key[0] = 15;
  r[42].key[0] = 18;
  r[43].key[0] = 19;
  r[44].key[0] = 21;
  r[45].key[0] = 23;
  r[46].key[0] = 24;
  r[47].key[0] = 25;

  RecordArr_t out(4);
  Device ssd("tests/ssd", 1, 1, 1);
  Device outssd("tests/outssd", 1, 1, 1);
  ssd.ewrite(reinterpret_cast<char *>((r + 32).data()), 2 * 8 * Record_t::bytes,
             0);
  Index_r index_r(8);
  inmem_spill_merge(r, {4, out}, {&ssd, &outssd}, index_r, {8, 4}, 2);

  outssd.eread(reinterpret_cast<char *>(r.data()), 6 * 8 * Record_t::bytes, 0);

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
  REQUIRE(r[21].key[0] == 11);
  REQUIRE(r[22].key[0] == 12);
  REQUIRE(r[23].key[0] == 13);
  REQUIRE(r[24].key[0] == 14);
  REQUIRE(r[25].key[0] == 15);
  REQUIRE(r[26].key[0] == 16);
  REQUIRE(r[27].key[0] == 17);
  REQUIRE(r[28].key[0] == 18);
  REQUIRE(r[29].key[0] == 19);
  REQUIRE(r[30].key[0] == 20);
  REQUIRE(r[31].key[0] == 21);
  REQUIRE(r[32].key[0] == 22);
  REQUIRE(r[33].key[0] == 23);
  REQUIRE(r[34].key[0] == 24);
  REQUIRE(r[35].key[0] == 25);
  REQUIRE(r[36].key[0] == 28);
  REQUIRE(r[37].key[0] == 31);
  REQUIRE(r[38].key[0] == 32);
  REQUIRE(r[39].key[0] == 33);
  REQUIRE(r[40].key[0] == 34);
  REQUIRE(r[41].key[0] == 35);
  REQUIRE(r[42].key[0] == 36);
  REQUIRE(r[43].key[0] == 37);
  REQUIRE(r[44].key[0] == 38);
  REQUIRE(r[45].key[0] == 54);
  REQUIRE(r[46].key[0] == 62);
  REQUIRE(r[47].key[0] == 78);
}
