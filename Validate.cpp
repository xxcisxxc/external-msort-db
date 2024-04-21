#include "Validate.h"
#include "Consts.h"
#include "Device.h"
#include "Record.h"
#include "defs.h"
#include <climits>

ValidatePlan::ValidatePlan(Plan *const input)
    : _input(input), _outputWitnessRecord(new Record_t),
      _inputWitnessRecord(_input->witnessRecord()),
      _buffer(_input->records().ptr(), 2) {
  TRACE(true);
  _outputWitnessRecord->fill(0);
} // ValidatePlan::ValidatePlan

ValidatePlan::~ValidatePlan() { TRACE(true); } // ValidatePlan::~ValidatePlan

Iterator *ValidatePlan::init() const {
  TRACE(true);
  return new ValidateIterator(this);
} // ValidatePlan::init

ValidateIterator::ValidateIterator(ValidatePlan const *const plan)
    : _count(0), _plan(plan), _input(plan->_input->init()), _out(kHDD) {
  TRACE(true);
} // ValidateIterator::ValidateIterator

ValidateIterator::~ValidateIterator() {
  TRACE(true);

  delete _input;
  traceprintf("validate %lu rows\n", (unsigned long)(_count));
} // ValidateIterator::~ValidateIterator

bool ValidateIterator::next() {
  TRACE(true);

  static bool generated = false;
  while (_input->next()) {
    generated = true;
  }

  static bool val_sorted = true;
  static uint8_t ind = 0;

  if (generated) {
    if (_count == 0) {
      const_cast<Record_t &>(_plan->_buffer[0]).fill(CHAR_MIN);
      const_cast<Record_t &>(_plan->_buffer[1]).fill(CHAR_MIN);
    }
    Record_t &buffer = const_cast<Record_t &>(_plan->_buffer[ind]);
    while (_out.read_only(buffer, Record_t::bytes) > 0) {
      ++_count;
      traceprintf("record: %d %d\n", buffer.key[0], buffer.key[1]);
      _plan->_outputWitnessRecord->x_or(buffer);
      ind ^= 1; // prev and next buffer
      if (buffer < _plan->_buffer[ind]) {
        traceprintf("record not sorted %ld: prev: %d %d, cur: %d %d\n",
                    _count - 1, _plan->_buffer[ind].key[0],
                    _plan->_buffer[ind].key[1], buffer.key[0], buffer.key[1]);
        val_sorted = false;
      }
      return generated;
    }

    bool val_witness =
        (*(_plan->_outputWitnessRecord.get()) == _plan->_inputWitnessRecord);

    if (val_witness && val_sorted)
      traceprintf("Witness: yes, Sorted: yes\n");
    else
      traceprintf("Witness: %s, Sorted %s\n", val_witness ? "yes" : "no",
                  val_sorted ? "yes" : "no");
    generated = false;
  }

  _plan->_outputWitnessRecord->fill(0);

  return generated;
} // ValidateIterator::next
