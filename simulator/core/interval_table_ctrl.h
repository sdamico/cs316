// File Name         : interval_table_ctrl.h
// Description       : Interval table controller hardware simulation class definitions
// Table of Contents : SubReadInterval struct definition
//                     IntervalTableCtrl class definition
//                       IntervalTableCtrl()         - Constructor
//                       ~IntervalTableCtrl()        - Destructor
//                       NextClockCycle()            - Advance to the next clock cycle
//                       Reset()                     - Reset internal states
//                       set_position_table_length() - Set position table length variable
//
// Revision History  :
//     Albert Ng      Nov 20 2012     Initial Revision

#ifndef CS316_CORE_INTERVAL_TABLE_CTRL_H_
#define CS316_CORE_INTERVAL_TABLE_CTRL_H_

#include <stdint.h>
#include "sequential.h"
#include "position_table_ctrl.h"
#include "fifo.h"
#include "params.h"
#include "def.h"
#include "input_reader.h"

// Simulates the Interval Table Controller (ITC). The ITC takes a subread from the
// Input Reader, looks up the start indices of the subread and the lexicographically
// next subread to obtain the start index and length of the position table interval
// for the subread. It passes the start index and length to the Position Table
// Controller, along with the subread information.
class IntervalTableCtrl : public Sequential {
 public:
  IntervalTableCtrl(uint64_t itc_id, InputReader* input_reader,
                    RamModule<uint32_t>* interval_table_ram, PositionTableCtrl* ptc,
                    unsigned int interval_table_size);
  ~IntervalTableCtrl();
  
  // Checks if an interval is ready for the PTC
  bool IntervalReady();
  
  // Gets the next ready interval
  SubReadInterval IntervalData();
  
  void NextClockCycle();
  void Reset();
 private:
  // Compute the number of interval table elements for each ram.
  void ComputeRamNumElem(unsigned int interval_table_size);
  
  // Compute the interval table ram address for a subread.
  uint64_t GetRamAddress(uint64_t subread);
  
  // ID number for this interval table controller. Used as port numbers to the
  // input reader and interval table ram module.
  uint64_t itc_id_;
  
  // Pointer to the interval table ram module from which to read pointer table
  // interval information.
  RamModule<uint32_t>* interval_table_ram_;
  
  // Pointer to the input reader from which to get subread work units.
  InputReader* input_reader_;
  
  // Pointer to the attached position table controller.
  PositionTableCtrl* ptc_;
  
  // FIFO containing outstanding interval table ram read requests.
  Fifo<SubRead>* interval_table_ram_fifo_;
  
  // Address for the second interval table read for a subread.
  uint64_t second_lookup_address_;
  
  // Flag indicating that the next interval table request is for the
  // first lookup for a subread.
  bool first_lookup_request_;
  
  // Flag indicating that the next ready interval table read output is the
  // first lookup for a subread.
  bool first_lookup_read_;
  
  // Storage of the first interval table lookup for a subread while waiting for the
  // second one.
  uint64_t first_lookup_data_;
  
  // FIFO containing subread and interval information ready to be sent to PTC.
  Fifo<SubReadInterval>* output_fifo_;
  
  // Number of interval table elements stored in each RAM
  unsigned int* ram_num_elem_;
};

IntervalTableCtrl::IntervalTableCtrl(uint64_t itc_id, InputReader* input_reader,
                                     RamModule<uint32_t>* interval_table_ram,
                                     PositionTableCtrl* ptc, unsigned int interval_table_size) {
  itc_id_ = itc_id;
  input_reader_ = input_reader;
  interval_table_ram_ = interval_table_ram;
  ptc_ = ptc;
  interval_table_ram_fifo_ = new Fifo<SubRead>(INTERVAL_TABLE_CTRL_FIFO_LENGTH);
  first_lookup_request_ = true;
  first_lookup_read_ = true;
  output_fifo_ = new Fifo<SubReadInterval>(INTERVAL_TABLE_CTRL_FIFO_LENGTH);
  ComputeRamNumElem(interval_table_size);
}

IntervalTableCtrl::~IntervalTableCtrl() {
  delete output_fifo_;
}

bool IntervalTableCtrl::IntervalReady() {
  return !output_fifo_->IsEmpty();
}

SubReadInterval IntervalTableCtrl::IntervalData() {
  SubReadInterval sri = output_fifo_->read_data();
  output_fifo_->ReadRequest();
  return sri;
}

void IntervalTableCtrl::NextClockCycle() {
  Sequential::NextClockCycle();
  
  // Get the next subread from the input reader, send the first interval table read request,
  // and compute the second read request address when:
  //   1. Input reader has a subread available
  //   2. The table lookup is the first lookup for this subread
  //   3. Not enough outstanding requests to fill the output FIFO
  //   4. Interval table ram module port is ready to accept another request
  if (input_reader_->SubReadReady(itc_id_) && first_lookup_request_ == true &&
      interval_table_ram_fifo_->Size() + output_fifo_->Size() < INTERVAL_TABLE_CTRL_FIFO_LENGTH &&
      interval_table_ram_->IsPortReady(itc_id_) == true) {
    SubRead sr = input_reader_->SubReadRequest(itc_id_);

    interval_table_ram_fifo_->WriteRequest(sr);
    interval_table_ram_->ReadRequest(GetRamAddress(sr.data), itc_id_);
    second_lookup_address_ = sr.data + 1;
    
    first_lookup_request_ = false;
  }  
  // Send the second interval table lookup if the interval table ram port is ready to accept
  // another request.
  else if (first_lookup_request_ == false && interval_table_ram_->IsPortReady(itc_id_) == true) {
    interval_table_ram_->ReadRequest(GetRamAddress(second_lookup_address_), itc_id_);
    first_lookup_request_ = true;
  }
  
  // Process the lookup when it is ready from the interval table ram module.
  // If it is the first lookup for a subread, store it.
  // If it is the second lookup for a subread, pop the subread info from the interval
  // table ram FIFO, grab the previously stored first lookup, compute the interval
  // length, and store the subread info and interval info to the output FIFO.
  if (interval_table_ram_->ReadReady(itc_id_)) {
    uint64_t lookup_data = interval_table_ram_->ReadData(itc_id_);
    if (first_lookup_read_ == true) {
      first_lookup_data_ = lookup_data;
      first_lookup_read_ = false;
    } else {
      SubRead sr = interval_table_ram_fifo_->read_data();
      interval_table_ram_fifo_->ReadRequest();
      
      PositionTableInterval pti;
      pti.start = first_lookup_data_;
      pti.length = lookup_data - first_lookup_data_;
      SubReadInterval sri;
      sri.sr = sr;
      sri.interval = pti;
      output_fifo_->WriteRequest(sri);
      
      first_lookup_read_ = true;
    }
  }
  
  interval_table_ram_fifo_->NextClockCycle();
  output_fifo_->NextClockCycle();
}

void IntervalTableCtrl::Reset() {
  output_fifo_->Reset();
}

void IntervalTableCtrl::ComputeRamNumElem(unsigned int interval_table_size) {
  unsigned int num_rams = interval_table_ram_->num_rams();
  ram_num_elem_ = new unsigned int[num_rams];
  for (unsigned int i = 0; i < num_rams; i++) {
    ram_num_elem_[i] = interval_table_size / num_rams;
  }
  for (unsigned int i = 0; i < interval_table_size % num_rams; i++) {
    ram_num_elem_[i]++;
  }
}
                                     
uint64_t IntervalTableCtrl::GetRamAddress(uint64_t subread) {
  unsigned int ram_id = 0;
  while (subread >= ram_num_elem_[ram_id]) {
    subread -= ram_num_elem_[ram_id];
    ram_id++;
  }
  return ((ram_id << interval_table_ram_->ram_address_width()) + subread);
}

#endif // CS316_CORE_INTERVAL_TABLE_CTRL_H_
