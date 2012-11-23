#ifndef CS316_CORE_POSITION_TABLE_CTRL_H_
#define CS316_CORE_POSITION_TABLE_CTRL_H_

#include <stdint.h>
#include "sequential.h"
#include "def.h"

class PositionTableCtrl : public Sequential {
public:
  PositionTableCtrl();
  ~PositionTableCtrl();

  bool ReceiveReady();
  void ReceiveInterval(SubReadInterval sri);
  
  bool received();
  SubReadInterval GetSRI();
  
  void NextClockCycle();
  void Reset();
  
private:
  bool received_;
  SubReadInterval sri_;
};

PositionTableCtrl::PositionTableCtrl() {
  received_ = false;
}

PositionTableCtrl::~PositionTableCtrl() {
}

bool PositionTableCtrl::ReceiveReady() {
  return true;
}

void PositionTableCtrl::ReceiveInterval(SubReadInterval sri) {
  received_ = true;
  sri_ = sri;
}

bool PositionTableCtrl::received() {
  return received_;
}

SubReadInterval PositionTableCtrl::GetSRI() {
  received_ = false;
  return sri_;
}

void PositionTableCtrl::NextClockCycle() {
  Sequential::NextClockCycle();
}

void PositionTableCtrl::Reset() {
}

#endif // CS316_CORE_POSITION_TABLE_CTRL_H_