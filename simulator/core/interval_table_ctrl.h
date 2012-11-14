// TODO(Albert): Complete implementation

#ifndef CS316_CORE_INTERVAL_TABLE_CTRL_H_
#define CS316_CORE_INTERVAL_TABLE_CTRL_H_

#include <cstdint>
#include "sequential.h"

class Interval_Table_Ctrl : public Sequential {
 public:
  Interval_Table_Ctrl();
  ~Interval_Table_Ctrl();
  void NextClockCycle();
  void Reset();    
 private:
};

#endif // CS316_CORE_INTERVAL_TABLE_CTRL_H_