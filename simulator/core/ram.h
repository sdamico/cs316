#ifndef CS316_CORE_RAM_H_
#define CS316_CORE_RAM_H_
// Simulates a 64-bit memory interface

#include <stdint.h>
#include <queue>
#include <cmath>
#include <assert.h>
#include "sequential.h"

template <typename T>
struct RamRequest {
  uint64_t address;
  T data;
  uint64_t age;
};

template <typename T>
class Ram : public Sequential {
 public:
  Ram();
  Ram(uint64_t address_width, uint8_t latency);
  ~Ram();
  void NextClockCycle();
  void Reset();
  void WriteRequest(uint64_t address, T data);
  void ReadRequest(uint64_t address);
  bool read_ready();
  T read_data();
 private:
  std::queue<RamRequest<T> > read_queue_;
  std::queue<RamRequest<T> > write_queue_;
  T *data_;
  uint64_t address_width_;
  uint8_t latency_;
  T read_data_;
  bool read_ready_;
};

template <typename T>
Ram<T>::Ram() {
  address_width_ = 0;
}

template <typename T>
Ram<T>::Ram(uint64_t address_width, uint8_t latency) {
  latency_ = latency;
  address_width_ = address_width;
  data_ = new T[(int) pow(2, address_width)];
  read_ready_ = false;
}

template <typename T>
Ram<T>::~Ram() {
  if (address_width_ > 0) {
    delete data_;
  }
}

template <typename T>
void Ram<T>::NextClockCycle() {
  Sequential::NextClockCycle();
  read_ready_ = false;
  
  if(!read_queue_.empty()) {
    if((read_queue_.front().age + latency_) <= cycle_count()) {
      RamRequest<T> ram_req = read_queue_.front();
      read_queue_.pop();
      read_ready_ = true;
      assert(ram_req.address < pow(2, address_width_));
      read_data_ = data_[ram_req.address];
    }
  }
  if(!write_queue_.empty()) {
    if((write_queue_.front().age + latency_) <= cycle_count()) {
      RamRequest<T> ram_req = write_queue_.front();
      write_queue_.pop();
      assert(ram_req.address < pow(2, address_width_)); 
      data_[ram_req.address] = ram_req.data;
    }
  }
}

// Currently just resets queue states
template <typename T>
void Ram<T>::Reset() {
  while(!read_queue_.empty()) {
    read_queue_.pop();
  }
  while(!write_queue_.empty()) {
    write_queue_.pop();
  }
  read_ready_ = false;
}

template <typename T>
void Ram<T>::WriteRequest(uint64_t address, T data)
{
  RamRequest<T> ram_req;
  ram_req.address = address;
  ram_req.data = data;
  ram_req.age = cycle_count();
  write_queue_.push(ram_req);
}

template <typename T>
void Ram<T>::ReadRequest(uint64_t address)
{
  RamRequest<T> ram_req;
  ram_req.address = address;
  ram_req.age = cycle_count();
  read_queue_.push(ram_req);
}

template <typename T>
bool Ram<T>::read_ready()
{
  return read_ready_;
}

template <typename T>
T Ram<T>::read_data()
{
  return read_data_;
}

#endif // CS316_CORE_RAM_H_
