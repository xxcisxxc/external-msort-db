#include "Iterator.h"
#include "Utils.h"
#include "defs.h"

Plan::Plan() { TRACE(true); } // Plan::Plan

Plan::~Plan() { TRACE(true); } // Plan::~Plan

Iterator::Iterator() : _count(0) { TRACE(true); } // Iterator::Iterator

Iterator::~Iterator() { TRACE(true); } // Iterator::~Iterator

void Iterator::run() {
  TRACE(true);

  while (next())
    ++_count;

  traceprintf("entire plan produced SORTED %lu%s rows\n", (unsigned long)_count,
              (isDistinct() ? " UNIQUE" : ""));
} // Iterator::run
