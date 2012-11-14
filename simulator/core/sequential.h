#ifndef CS316_CORE_SEQUENTIAL_H_
#define CS316_CORE_SEQUENTIAL_H_

#include <stdint.h>         // (Albert): Changed from cstdint to stdint.h

class Sequential
{
 public:
  Sequential() {
    cycle_count_ = 0;
  }
  virtual ~Sequential(){};  // (Albert): Changed from pure virtual to empty virtual
  void NextClockCycle() {
    cycle_count_++;
  }
  uint64_t cycle_count() {
    return cycle_count_;
  }
  virtual void Reset() = 0;
 private:
  uint64_t cycle_count_;
};

#endif // CS316_CORE_SEQUENTIAL_H_
