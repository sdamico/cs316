#ifndef CS316_CORE_RAM_H_
// Simulates a 64-bit memory interface

#include <cstdint>
#include <queue>

struct RamRequest {
  uint64_t address;
  uint64_t data;
  uint8_t age;
};

class Ram {
 public:
  Ram();
  Ram(uint64_t address_width, uint8_t read_latency, uint8_t write_latency);
  ~Ram();
  void NextClockCycle();
  void Reset();
  void Write(uint64_t address, uint64_t data);  
  void Read(uint64_t address, uint64_t *data);

 private:
  std::queue<RamRequest> read_queue_;
  std::queue<RamRequest> write_queue_;
  uint64_t *data_;
  uint64_t address_width_;
  uint8_t read_latency_;
};

#endif // CS316_CORE_RAM_H_
