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
                    RamModule<uint32_t>* interval_table_ram, unsigned int interval_table_size);
  ~IntervalTableCtrl();
  
  // Checks if an interval is ready for the PTC
  bool IntervalReady();
  
  // Gets the next ready interval
  SubReadInterval IntervalData();
  
  // Check if no work currently in the pipeline
  bool IsIdle();
  
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
  
  // Number of interval table elements stored in each RAM and bank
  unsigned int** ram_bank_num_elem_;
};

IntervalTableCtrl::IntervalTableCtrl(uint64_t itc_id, InputReader* input_reader,
                                     RamModule<uint32_t>* interval_table_ram,
                                     unsigned int interval_table_size) {
  itc_id_ = itc_id;
  input_reader_ = input_reader;
  interval_table_ram_ = interval_table_ram;
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

bool IntervalTableCtrl::IsIdle() {
  if (interval_table_ram_fifo_->IsEmpty() && output_fifo_->IsEmpty()) {
    return true;
  } else {
    return false;
  }
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
      /*if (pti.length > 100) {
        pti.length = 0;
        num_skipped_subreads++;
      }*/
      SubReadInterval sri;
      sri.sr = sr;
      sri.interval = pti;
      output_fifo_->WriteRequest(sri);
      //std::cout << "cc interval.start interval.length " << cycle_count() << " " << sri.interval.start << " " << sri.interval.length << " " << std::endl;
      //std::cout << "cc sr read_id subread_offset length data " << cycle_count() << " " << sri.sr.read_id << " " << sri.sr.subread_offset << " " << " " << sri.sr.length << " " << sri.sr.data << std::endl;
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
  ram_bank_num_elem_ = new unsigned int*[INTERVAL_TABLE_CTRL_NUM_RAMS];
  for (unsigned int i = 0; i < INTERVAL_TABLE_CTRL_NUM_RAMS; i++) {
    ram_bank_num_elem_[i] = new unsigned int[(int) pow(2, INTERVAL_TABLE_CTRL_RAM_ADDR_BANK_WIDTH)];
    for (unsigned int j = 0; j < pow(2, INTERVAL_TABLE_CTRL_RAM_ADDR_BANK_WIDTH); j++) {
      ram_bank_num_elem_[i][j] = interval_table_size / INTERVAL_TABLE_CTRL_NUM_RAMS / ((unsigned int) pow(2, INTERVAL_TABLE_CTRL_RAM_ADDR_BANK_WIDTH));
    }
  }
  for (unsigned int i = 0; i < interval_table_size % (INTERVAL_TABLE_CTRL_NUM_RAMS * ((unsigned int) pow(2, INTERVAL_TABLE_CTRL_RAM_ADDR_BANK_WIDTH))); i++) {
    ram_bank_num_elem_[i / ((unsigned int) pow(2, INTERVAL_TABLE_CTRL_RAM_ADDR_BANK_WIDTH))][i % ((unsigned int) pow(2, INTERVAL_TABLE_CTRL_RAM_ADDR_BANK_WIDTH))]++;
  }
}

uint64_t IntervalTableCtrl::GetRamAddress(uint64_t subread) {
  unsigned int ram_id = 0;
  unsigned int bank_id = 0;
  while (subread >= ram_bank_num_elem_[ram_id][bank_id]) {
    subread -= ram_bank_num_elem_[ram_id][bank_id];
    bank_id++;
    if (bank_id == pow(2, INTERVAL_TABLE_CTRL_RAM_ADDR_BANK_WIDTH)) {
      ram_id++;
      bank_id = 0;
    }
  }
  return ((ram_id << INTERVAL_TABLE_CTRL_RAM_ADDR_WIDTH) +
          (bank_id << (INTERVAL_TABLE_CTRL_RAM_ADDR_ROW_WIDTH + INTERVAL_TABLE_CTRL_RAM_ADDR_COL_WIDTH)) +
          subread);
}

#endif // CS316_CORE_INTERVAL_TABLE_CTRL_H_
