#include "Filter.h"
#include "Iterator.h"
#include "Record.h"
#include "Scan.h"
#include "Sort.h"
#include "defs.h"

constexpr std::size_t kCount = 256;
constexpr std::size_t kSize = 20;

int main(/*int argc, char *argv[]*/) {
  TRACE(true);

  Record_t::bytes = kSize;

  // Plan * const plan = new ScanPlan (7);
  // Plan *const plan = new SortPlan(new FilterPlan(new ScanPlan(kCount)));
  Plan *const plan = new SortPlan(new ScanPlan(kCount));

  Iterator *const it = plan->init();
  it->run();
  delete it;

  // const RecordArr_t &records = plan->records();
  RecordArr_t records(kCount);
  std::fstream file;
  file.open("hdd", std::ios::in | std::ios::binary);
  file.read(reinterpret_cast<char *>(records.data()), kCount * kSize);
  file.close();
  for (uint32_t i = 0; i < kCount; ++i)
    traceprintf("record %d: %d %d\n", i, records[i].key[0], records[i].key[1]);

  delete plan;

  return 0;
} // main
