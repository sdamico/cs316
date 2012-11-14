// TODO(Albert): Complete implementation

#ifndef CS316_CORE_RAM_ARBITER_H_
#define CS316_CORE_RAM_ARBITER_H_

#include <cstdint>
#include "sequential.h"

class Ram_Arbiter : public Sequential {
 public:
  Ram_Arbiter();
  ~Ram_Arbiter();
  void NextClockCycle();
  void Reset();
 private:
};

#endif // CS316_CORE_RAM_ARBITER_H_