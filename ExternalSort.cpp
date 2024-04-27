#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#include "Iterator.h"
#include "Record.h"
#include "Scan.h"
#include "Sort.h"
#include "Utils.h"
#include "Validate.h"
#include "defs.h"

int main(int argc, char *argv[]) {
  TRACE(true);

  std::size_t nRecords = 0;
  std::string tracefile = "trace.log";

  for (int i = 1; i < argc; ++i) {
    if (std::string(argv[i]) == "-c") {
      nRecords = std::stoul(argv[++i]);
    } else if (std::string(argv[i]) == "-s") {
      Record_t::bytes = std::stoul(argv[++i]);
    } else if (std::string(argv[i]) == "-o") {
      tracefile = argv[++i];
    } else {
      throw std::invalid_argument("unknown option");
    }
  } // for

  auto file_logger = spdlog::basic_logger_mt("basic_logger", tracefile, true);
  spdlog::set_default_logger(file_logger);
  spdlog::set_pattern("[%H:%M:%S %z][%^%l%$] %v");

  printf("=======================\n");
  printf("# of records: %lu\n", nRecords);
  printf("# of bytes in record: %lu\n", Record_t::bytes);
  printf("# of records in one cache run: %lu\n", cache_nrecords());
  printf("# of cache runs in memory: %lu\n", mem_nruns());
  printf("# of records in one memory run: %lu\n", mem_nrecords());
  printf("# of records in output buffer: %lu\n", out_nrecords());
  printf("# of memory runs in SSD: %lu\n", ssd_nruns());
  printf("# of records in one SSD run: %lu\n", ssd_nrecords());
  printf("=======================\n");

  Plan *const plan = new ValidatePlan(new SortPlan(new ScanPlan(nRecords)));

  Iterator *const it = plan->init();
  it->run();
  delete it;

  delete plan;

  spdlog::shutdown();

  return 0;
} // main
