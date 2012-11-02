#ifndef CS316_CORE_REGISTER_H_
#define CS316_CORE_REGISTER_H_
#include <cstdint>
#include "sequential.h"

class Register : Public Sequential {
 public:
  Register();
  Register(uint32_t size);
  ~Register();
  void NextClockCycle();
  void Reset();
  void SetInput(uint32_t index, uint32_t data);  
  void SetInput(uint32_t index, uint32_t data, uint32_t mask);
  void GetOutput(uint32_t index, uint32_t &data);

 private:
  uint32_t *d_values_;
  uint32_t *q_values_;
};

#endif // CS316_CORE_REGISTER_H_
