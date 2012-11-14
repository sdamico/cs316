// File Name            : fifo.h
// Description          : FIFO hardware simulation class definitions
// Table of Contents    : Fifo class declaration
//                        Fifo::Fifo()           - Constructor 
//                        Fifo::~Fifo()          - Destructor
//                        Fifo::IsFull()         - Check if Fifo is full
//                        Fifo::IsEmpty()        - Check if Fifo is empty
//                        Fifo::WriteRequest()   - Request to write data to Fifo
//                        Fifo::ReadRequest()    - Request to read data from Fifo
//                        Fifo::NextClockCycle() - Advance to the next clock cycle
// Revision History     :
//     Albert Ng      Nov 13 2012     Initial Revision

#ifndef CS316_CORE_FIFO_H_
#define CS316_CORE_FIFO_H_

#include <stdint.h>
#include <queue>
#include "sequential.h"

// Simulates a general hardware FIFO with a set length and data type.
template <class T>
class Fifo : public Sequential {
 public:
  Fifo(uint64_t fifo_length);
  ~Fifo();
  
  // Returns true if FIFO cannot hold any more entries.
  bool IsFull();
  
  // Returns true if FIFO does not hold any entries.
  bool IsEmpty();
  
  // Request to write the passed data on next clock cycle.
  // It is up to the requestor to check if the FIFO is full.
  // If FIFO is full upon request, the request is ignored.
  // Data is written when NextClockCycle() is called.
  void WriteRequest(T write_data);
  
  // Request to read data on next clock cycle and store to the passed address.
  // It is up to the requestor to check if the FIFO is empty.
  // If FIFO is empty, the request is ignored.
  // Data is read out when NextClockCycle() is called.
  void ReadRequest(T *read_location);
  
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
  
  // FIFO write requested in current clock cycle.
  bool write_requested_;
  
  // FIFO read requested in current clock cycle.
  bool read_requested_;
  
  // Location to store FIFO read data on next clock cycle.
  T* read_location_;
  
  // Data to write to FIFO on next clock cycle.
  T write_data_;
};

template <class T>
Fifo<T>::Fifo(uint64_t fifo_length) {
  fifo_length_ = fifo_length;
  write_requested_ = false;
  read_requested_ = false;
}

template <class T>
Fifo<T>::~Fifo() {
}

// Returns true if the data queue length has reached the set FIFO length
template <class T>
bool Fifo<T>::IsFull() {
  return data_.size() == fifo_length_;
}

template <class T>
bool Fifo<T>::IsEmpty() {
  return data_.empty();
}

template <class T>
void Fifo<T>::WriteRequest(T write_data) {
  if (!IsFull()) {
    write_requested_ = true;
    write_data_ = write_data;
  }
}

template <class T>
void Fifo<T>::ReadRequest(T *read_location) {
  if (!IsEmpty()) {
    read_requested_ = true;
    read_location_ = read_location;
  }
}

template <class T>
void Fifo<T>::NextClockCycle() {
  if (write_requested_) {
    data_.push(write_data_);
    write_requested_ = false;  
  }
  
  if (read_requested_) {
    *read_location_ = data_.front();
    data_.pop();
    read_requested_ = false;
  }
}

template <class T>
void Fifo<T>::Reset() {
  while (!data_.empty()) {
    data_.pop();
  }
  write_requested_ = false;
  read_requested_ = false;
}

#endif // CS316_CORE_FIFO_H_