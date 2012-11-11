#include <cmath>
#include "ram.h"

Ram::Ram() {
}

Ram::Ram(uint64_t address_width, uint8_t read_latency, uint8_t write_latency) {
  read_latency_ = read_latency;
  write_latency_ = write_latency;
  address_width_ = address_width;
  data = new uint64_t[pow(2, address_width)];
}

Ram::~Ram() {
  delete data;
}

void Ram::NextClockCycle() {
  Sequential::NextClockCycle();
  if(!read_queue_.empty()) {
    if((read_queue_.front().age+read_latency_) <= cycle_count()) {
      RamRequest ram_req = read_queue_.pop();
      // TODO(sdamico): make this safe?
      read_valid_ = true;
      read_data_ = data_[ram_req.address];
    }
  }
  if(!write_queue_.empty()) {
    if((write_queue_.front().age+write_latency_) <= cycle_count()) {
      RamRequest ram_req = write_queue_.pop();
      // TODO(sdamico): make this safe?
      data_[ram_req.address] = ram_req.data;
    }
  }

}

// Currently just resets queue states
void Ram::Reset() {
  while(!read_queue_.empty())
    read_queue_.pop();
  while(!write_queue_.empty())
    write_queue_.pop();
}

void Ram::Write(uint64_t address, uint64_t data)
{
  RamRequest ram_req;
  ram_req.address = address;
  ram_req.data = data;
  ram_req.age = cycle_count();
  write_queue_.push(ram_req);
}

void Ram::ReadRequest(uint64_t address)
{
  RamRequest ram_req;
  ram_req.address = address;
  ram_req.age = cycle_count();
  read_queue_.push(ram_req);
}

uint64_t Ram::read_valid()
{
  return read_valid_;
}

uint64_t Ram::read_data()
{
  read_valid_ = false;
  return read_data_;  
}
