// TODO(Albert): Complete implementation

#ifndef CS316_CORE_POSITION_TABLE_CTRL_H_
#define CS316_CORE_POSITION_TABLE_CTRL_H_

#include <stdint.h>
#include "sequential.h"
#include "def.h"

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
  void ReceiveInterval(PositionTableInterval interval, SubRead sr);
  
 private:
};

bool PositionTableCtrl::ReceiveReady() {
  return true;
}

void PositionTableCtrl::ReceiveInterval(PositionTableInterval interval,
                                        SubRead sr) {
  return;
}

#endif // CS316_CORE_POSITION_TABLE_CTRL_H_