#include "Filter.h"
#include "Iterator.h"
#include "Record.h"
#include "Scan.h"
#include "Sort.h"
#include "Validate.h"
#include "defs.h"

constexpr std::size_t kCount = 66;
constexpr std::size_t kSize = 20;

int main(/*int argc, char *argv[]*/) {
  TRACE(true);

  Record_t::bytes = kSize;

  // Plan * const plan = new ScanPlan (7);
  // Plan *const plan = new SortPlan(new FilterPlan(new ScanPlan(kCount)));
  Plan *const plan = new ValidatePlan(new SortPlan(new ScanPlan(kCount)));

  Iterator *const it = plan->init();
  it->run();
  delete it;

  delete plan;

  return 0;
} // main
