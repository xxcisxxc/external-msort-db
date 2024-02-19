#include "Filter.h"
#include "Iterator.h"
#include "Record.h"
#include "Scan.h"
#include "Sort.h"

constexpr std::size_t kCount = 8;
constexpr std::size_t kSize = 8;

int main(/*int argc, char *argv[]*/) {
  TRACE(true);

  Record_t::bytes = kSize;

  // Plan * const plan = new ScanPlan (7);
  Plan *const plan = new SortPlan(new FilterPlan(new ScanPlan(kCount)));

  Iterator *const it = plan->init();
  it->run();
  delete it;

  const RecordArr_t &records = plan->records();
  for (uint32_t i = 0; i < kCount; ++i)
    traceprintf("record %d: %d\n", i, records[i].key);

  delete plan;

  return 0;
} // main
