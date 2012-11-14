// TODO(Albert): Complete implementation

#ifndef CS316_CORE_STITCHER_H_
#define CS316_CORE_STITCHER_H_

#include <cstdint>
#include "sequential.h"

class Stitcher : public Sequential {
 public:
  Stitcher();
  ~Stitcher();
  void NextClockCycle();
  void Reset();    
 private:
};

#endif // CS316_CORE_STITCHER_H_