#ifndef CS316_CORE_RAM_H_
#define CS316_CORE_RAM_H_
// Simulates a 64-bit memory interface

#include <cstdint>
#include <queue>
#include "sequential.h"

struct RamRequest {
  uint64_t address;
  uint64_t data;
  uint8_t age;
};

class Ram : public Sequential {
 public:
  Ram();
  Ram(uint64_t address_width, uint8_t read_latency, uint8_t write_latency);
  ~Ram();
  void NextClockCycle();
  void Reset();
  void Write(uint64_t address, uint64_t data);  
  void ReadRequest(uint64_t address);
  bool read_ready();
  uint64_t read_data();
 private:
  std::queue<RamRequest> read_queue_;
  std::queue<RamRequest> write_queue_;
  uint64_t *data_;
  uint64_t address_width_;
  uint8_t read_latency_;
  uint64_t read_data_
  bool read_ready_;
};

#endif // CS316_CORE_RAM_H_
