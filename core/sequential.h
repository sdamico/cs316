#ifndef CS316_CORE_SEQUENTIAL_H_
#define CS316_CORE_SEQUENTIAL_H_

#include <cstdint>

class Sequential
{
 public:
  Sequential() {
    cycle_count_ = 0;
  }
  virtual ~Sequential() = NULL;
  void NextClockCycle() {
    cycle_count_++;
  }
  uint64_t cycle_count() {
    return cycle_count_;
  }
  virtual void Reset() = NULL;
 private:
  uint64_t cycle_count_;
};

#endif // CS316_CORE_SEQUENTIAL_H_
