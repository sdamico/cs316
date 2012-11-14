// TODO(Albert): Complete implementation

#ifndef CS316_CORE_POSITION_TABLE_CTRL_H_
#define CS316_CORE_POSITION_TABLE_CTRL_H_

#include <cstdint>
#include "sequential.h"

class Position_Table_Ctrl : public Sequential {
 public:
  Position_Table_Ctrl();
  ~Position_Table_Ctrl();
  void NextClockCycle();
  void Reset();    
 private:
};

#endif // CS316_CORE_POSITION_TABLE_CTRL_H_