// TODO(Albert): Complete implementation

#ifndef CS316_CORE_POSITION_TABLE_CTRL_H_
#define CS316_CORE_POSITION_TABLE_CTRL_H_

#include <stdint.h>
#include "sequential.h"

class PositionTableCtrl : public Sequential {
 public:
  PositionTableCtrl();
  ~PositionTableCtrl();
  void NextClockCycle();
  void Reset();
  
  // Interval Table Controller interface function stubs
  
  // Returns true if PTC is ready to receive a new interval
  bool ReceiveReady();
  
  // Receive and store the interval info for processing
  void ReceiveInterval(uint32_t interval_start, uint32_t interval_length)
  
 private:
};

bool PositionTableCtrl::ReceiveReady() {
  return true;
}

void PositionTableCtrl::ReceiveInterval(uint32_t interval_start, uint32_t interval_length) {
  return;
}

#endif // CS316_CORE_POSITION_TABLE_CTRL_H_