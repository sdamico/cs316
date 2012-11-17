// File Name         : fifo.h
// Description       : FIFO hardware simulation class definitions
// Table of Contents : Fifo class declaration
//                     Fifo::Fifo()           - Constructor
//                     Fifo::~Fifo()          - Destructor
//                     Fifo::IsFull()         - Check if FIFO is full
//                     Fifo::IsAlmostFull()   - Check if FIFO is almost full
//                     Fifo::IsEmpty()        - Check if FIFO is empty
//                     Fifo::WriteRequest()   - Request to write data to FIFO
//                     Fifo::ReadRequest()    - Request to read data from FIFO
//                     Fifo::NextClockCycle() - Advance to the next clock cycle
//                     Fifo::Reset()          - Reset FIFO state
// Revision History  :
//     Albert Ng      Nov 13 2012     Initial Revision
//     Albert Ng      Nov 14 2012     Added read_ready and IsAlmostFull()

#ifndef CS316_CORE_FIFO_H_
#define CS316_CORE_FIFO_H_

#include <stdint.h>
#include <queue>
#include <assert.h>
#include "sequential.h"
#include <iostream>

// Simulates a general hardware FIFO with a set length and data type.
template <typename T>
class Fifo : public Sequential {
 public:
  Fifo();
  Fifo(uint64_t fifo_length);
  Fifo(uint64_t fifo_length, uint64_t fifo_almost_full_length);
  ~Fifo();
  
  // Returns true if FIFO cannot hold any more entries.
  bool IsFull();
  
  // Returns true if FIFO is almost full.
  bool IsAlmostFull();
  
  // Returns true if FIFO does not hold any entries.
  bool IsEmpty();
  
  // Request to write the passed data on next clock cycle.
  // It is up to the requestor to check if the FIFO is full.
  // If FIFO is full upon request, assertion is raised.
  // Data is written when NextClockCycle() is called.
  void WriteRequest(T write_data);
  
  // Request to read data on next clock cycle.
  // It is up to the requestor to check if the FIFO is empty.
  // If FIFO is empty, assertion is raised.
  // Data is read out when NextClockCycle() is called.
  void ReadRequest();
  
  // Read data accessor
  T read_data();
  
  // Advance to the next clock cycle.
  // Services the read and/or write request from the previous clock cycle.
  void NextClockCycle();
  
  // Resets the FIFO to be empty, and clears the pending requests.
  void Reset();
  
 private:
  // Underlying data storage for the FIFO.
  std::queue<T> data_;
  
  // Fixed length of the FIFO, set in the constructor.
  uint64_t fifo_length_;
  
  // Capacity at which FIFO is considered almost full.
  uint64_t fifo_almost_full_length_;
  
  // FIFO write requested in current clock cycle.
  bool write_requested_;
  
  // FIFO read requested in current clock cycle.
  bool read_requested_;
  
  // Data to write to FIFO on next clock cycle.
  T write_data_;
  
  // Data to be read from ReadRequest() caller
  T read_data_;
};

template <typename T>
Fifo<T>::Fifo() {
}

template <typename T>
Fifo<T>::Fifo(uint64_t fifo_length) {
  fifo_length_ = fifo_length;
  fifo_almost_full_length_ = fifo_length;
  write_requested_ = false;
  read_requested_ = false;
}

template <typename T>
Fifo<T>::Fifo(uint64_t fifo_length, uint64_t fifo_almost_full_length) {
  fifo_length_ = fifo_length;
  fifo_almost_full_length_ = fifo_almost_full_length;
  assert(fifo_length_ >= fifo_almost_full_length_);
  write_requested_ = false;
  read_requested_ = false;
}

template <typename T>
Fifo<T>::~Fifo() {
}

// Returns true if the data queue length has reached the set FIFO length
template <typename T>
bool Fifo<T>::IsFull() {
  return data_.size() == fifo_length_;
}

template <typename T>
bool Fifo<T>::IsAlmostFull() {
  return data_.size() >= fifo_almost_full_length_;
}

template <typename T>
bool Fifo<T>::IsEmpty() {
  return data_.empty();
}

template <typename T>
void Fifo<T>::WriteRequest(T write_data) {
  if (IsFull()) {
    std::cerr << "Writing to Full FIFO!" << std::endl;
  }
  assert(!IsFull());
  write_requested_ = true;
  write_data_ = write_data;
}

template <typename T>
void Fifo<T>::ReadRequest() {
  if (IsEmpty()) {
    std::cerr << "Reading from Empty FIFO!" << std::endl;
  }
  assert(!IsEmpty());
  read_requested_ = true;
}

template <typename T>
void Fifo<T>::NextClockCycle() {
  Sequential::NextClockCycle();
  
  if (write_requested_) {
    data_.push(write_data_);
    write_requested_ = false;  
  }
  
  if (read_requested_) {
    data_.pop();
    read_requested_ = false;
  }
  
  if (!IsEmpty()) {
    read_data_ = data_.front();
  }
}

template <typename T>
void Fifo<T>::Reset() {
  while (!data_.empty()) {
    data_.pop();
  }

  write_requested_ = false;
  read_requested_ = false;
}

template <typename T>
T Fifo<T>::read_data() {
  return read_data_;
}

#endif // CS316_CORE_FIFO_H_