#ifndef CS316_CORE_RAM_H_
#define CS316_CORE_RAM_H_
// Simulates a 64-bit memory interface

#include <stdint.h>
#include <list>
#include <cmath>
#include <assert.h>
#include "sequential.h"

template <typename T>
struct RamRequest {
  uint64_t address;
  T data;
  int64_t wait_cycles;
};

struct BankState {
  uint64_t latency;
  uint64_t open_row;
};

template <typename T>
class Ram : public Sequential {
 public:
  Ram();
  Ram(uint64_t addr_row_width, uint64_t addr_col_width, uint64_t addr_bank_width, 
    uint64_t system_clock_freq_mhz, uint64_t memory_clock_freq_mhz,
    uint64_t tRCD_cycles, uint64_t tCL_cycles, uint64_t tRP_cycles);
  ~Ram();
  void NextClockCycle();
  void Reset();
  void DirectWrite(uint64_t address, T data);
  void WriteRequest(uint64_t address, T data);
  void ReadRequest(uint64_t address);
  bool read_ready();
  T read_data();
  static const int BURST_LENGTH = 8;
 private:
  std::list<RamRequest<T> > read_queue_;
  std::list<RamRequest<T> > write_queue_;
  T *data_;
  T read_data_;
  bool read_ready_;
  
  uint64_t addr_row_width_;
  uint64_t addr_col_width_;
  uint64_t addr_bank_width_;
  uint64_t addr_row_mask_;
  BankState* bank_states_;
  uint64_t num_banks_;
  
  //timing parameters (in picoseconds)
  uint64_t tRCD;    //row to column delay 
  uint64_t tCL;     //column to data delay
  uint64_t tRP;     //precharge to row delay
  uint64_t tBurst;  //time for data to be clocked out
  uint64_t system_clock_period_ps;  
  
  uint64_t time_since_last_read;
};

template <typename T>
Ram<T>::Ram() {
}

template <typename T>
Ram<T>::Ram(uint64_t addr_row_width, uint64_t addr_col_width, uint64_t addr_bank_width, 
    uint64_t system_clock_freq_mhz, uint64_t memory_clock_freq_mhz,
    uint64_t tRCD_cycles, uint64_t tCL_cycles, uint64_t tRP_cycles) {
  
  addr_row_width_ = addr_row_width;
  addr_col_width_ = addr_col_width;
  addr_bank_width_ = addr_bank_width;
  addr_row_mask_ = ((1 << addr_row_width_) - 1) << addr_col_width_;
  data_ = new T[(int) pow(2, addr_row_width+addr_bank_width+addr_col_width)];
  read_ready_ = false;
  
  num_banks_ = (uint64_t)pow(2, addr_bank_width_);
  bank_states_ = new BankState[num_banks_];
  for (unsigned int i = 0; i < num_banks_; i++) {
    bank_states_[i].latency = 0;
    bank_states_[i].open_row = -1;
  }
  
  //calculate timing paramters
  uint64_t memory_clock_period_ps = 1000000/memory_clock_freq_mhz;
  system_clock_period_ps = 1000000/system_clock_freq_mhz;
  tRCD = tRCD_cycles*memory_clock_period_ps;
  tCL = tCL_cycles*memory_clock_period_ps;
  tRP = tRP_cycles*memory_clock_period_ps;
  //assuming DDR, and burst length of BURST_LENGTH
  tBurst = BURST_LENGTH/2*memory_clock_period_ps;  
  time_since_last_read = 0;
}

template <typename T>
Ram<T>::~Ram() {
  delete[] data_;
  delete[] bank_states_;
}

template <typename T>
void Ram<T>::NextClockCycle() {
  Sequential::NextClockCycle();
  read_ready_ = false;
  
  if(!read_queue_.empty()) {
    if (read_queue_.front().wait_cycles <= 0 && time_since_last_read >= tBurst) {
      RamRequest<T> ram_req = read_queue_.front();
      read_queue_.pop_front();
      read_ready_ = true;
      read_data_ = data_[ram_req.address];
      time_since_last_read=0;
    }    
  }
  
  // Assume separate write port
  if(!write_queue_.empty()) {
    if(write_queue_.front().wait_cycles <= 0 ) {
      RamRequest<T> ram_req = write_queue_.front();
      write_queue_.pop_front();      
      data_[ram_req.address] = ram_req.data;      
    }
  }
  
  //iterate through read and write queues and decrement cycle counts  
  for (typename std::list< RamRequest<T> >::iterator it = read_queue_.begin(); it != read_queue_.end(); it++) {
    it->wait_cycles -= system_clock_period_ps;
  }
  for (typename std::list< RamRequest<T> >::iterator it = write_queue_.begin(); it != write_queue_.end(); it++) {
    it->wait_cycles -= system_clock_period_ps;
  }  
  for (unsigned int i = 0; i < num_banks_; i++) {
    if (bank_states_[i].latency > 0) {
      bank_states_[i].latency -= system_clock_period_ps;
    }
    else {
      bank_states_[i].latency = 0;
    }
  }
  time_since_last_read += system_clock_period_ps;
}

// Currently just resets queue states
template <typename T>
void Ram<T>::Reset() {
  while(!read_queue_.empty()) {
    read_queue_.pop_front();
  }
  while(!write_queue_.empty()) {
    write_queue_.pop_front();
  }
  read_ready_ = false;
  time_since_last_read = 0;
  for (unsigned int i = 0; i < num_banks_; i++) {
    bank_states_[i].latency = 0;
    bank_states_[i].open_row = -1;
  }
}

template <typename T>
void Ram<T>::DirectWrite(uint64_t address, T data) {
  assert(address < pow(2, addr_bank_width_ + addr_row_width_ + addr_col_width_));
  data_[address] = data;
}

template <typename T>
void Ram<T>::WriteRequest(uint64_t address, T data) {
  assert(address < pow(2, addr_bank_width_ + addr_row_width_ + addr_col_width_)); 
  RamRequest<T> ram_req;
  ram_req.address = address;
  ram_req.data = data;
  
  uint64_t row = address & addr_row_mask_;
  uint64_t bank = (address >> (addr_col_width_ + addr_row_width_));
  ram_req.wait_cycles = (bank_states_[bank].latency > 0) ? bank_states_[bank].latency : 0;
  if (row == bank_states_[bank].open_row) {
    ram_req.wait_cycles += tBurst;//same row latency
  }
  else {
    //wait for data to come out (tCL), wait for precharge (tRP), activate row (tRCD) 
    ram_req.wait_cycles += tCL+tRP+tRCD;//different row latency    
  }    
  write_queue_.push_back(ram_req);
  //update bank state
  bank_states_[bank].open_row = row;
  bank_states_[bank].latency = ram_req.wait_cycles;
}

template <typename T>
void Ram<T>::ReadRequest(uint64_t address)
{
  assert(address < pow(2, addr_bank_width_ + addr_row_width_ + addr_col_width_));
  
  //build up request meta data
  RamRequest<T> ram_req;
  ram_req.address = address;
  uint64_t row = address & addr_row_mask_;
  uint64_t bank = (address >> (addr_col_width_ + addr_row_width_));
  ram_req.wait_cycles = (bank_states_[bank].latency > 0) ? bank_states_[bank].latency : 0;
  if (row == bank_states_[bank].open_row) {
    ram_req.wait_cycles += tBurst;//same row latency
  }
  else {
    //wait for data to come out (tCL), wait for precharge (tRP), activate row (tRCD) 
    ram_req.wait_cycles += tCL+tRP+tRCD;//different row latency    
  }
  
  read_queue_.push_back(ram_req);

  //update bank state
  bank_states_[bank].open_row = row;
  bank_states_[bank].latency = ram_req.wait_cycles;
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
