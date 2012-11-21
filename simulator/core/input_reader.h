// File Name         : input_reader.h
// Description       : Input RAM reader hardware simulation class definitions
// Table of Contents : SubRead struct definition
//                     InputReader class definition
//                     InputReader::InputReader()    - Constructor
//                     InputReader::~InputReader()   - Destructor
//                     InputReader::SubReadRequest() - Request next work unit
//                     InputReader::SubReadReady()   - Check if next work unit ready
//                     InputReader::Done()           - Check if workload is finished
//                     InputReader::NextClockCycle() - Advance to next clock cycle
//                     InputReader::Reset()          - Reset internal states
//
// Revision History  :
//     Albert Ng      Nov 19 2012     Initial Revision
//     Albert Ng      Nov 20 2012     Debugged and passes unit test

#ifndef CS316_CORE_INPUT_READER_H_
#define CS316_CORE_INPUT_READER_H_

#include <stdint.h>
#include <fstream>
#include <cmath>
#include <assert.h>
#include "sequential.h"
#include "params.h"
#include "fifo.h"
#include "ram_module.h"
#include "def.h"

// Simulates the input workload RAM and the front-end reader that supplies the
// interval table controllers with subreads.
class InputReader : public Sequential {
 public:
  // Preloads subread input RAM module and prepares the subread fifos.
  // Asserts that the number of interval table controllers is an integer
  // multiple of the number of subreads per read.
  InputReader(char* subread_filename, uint64_t num_itcs);
  ~InputReader();
  
  // Returns the next valid subread for a given interval table controller.
  // It is up to the requestor to check if a subread is ready.
  // If no subread is ready upon request, assertion is raised.
  // Subread is popped off of the FIFO when NextClockCycle() is called.
  SubRead SubReadRequest(uint64_t itc_id);
  
  // Check if the subread data is ready for a given interval table controller.
  bool SubReadReady(uint64_t itc_id);
  
  // Check if workload is finished.
  bool Done();
  
  // Performs next clock logic
  void NextClockCycle();
  
  // Reset all internal states
  void Reset();
  
 private:
  // Number of full reads for this workload.
  uint64_t num_reads_;
  
  // Number of subreads per full read.
  uint64_t num_subreads_per_read_;
  
  // Number of interval table controllers attached to this reader.
  uint64_t num_itcs_;
  
  // Number of nucleotides per subread
  uint64_t subread_length_;
  
  // Read counters for each interval table controller.
  uint64_t* read_counters_;
  
  // RAM Module that contains the workload.
  RamModule<uint64_t>* input_ram_;
  
  // FIFOs that contain the available subreads for each interval table controller.
  Fifo<SubRead>** subread_fifos_;
  
  // FIFOs that contain the outstanding RAM read requests for each interval table
  // controller FIFO.
  Fifo<uint64_t>** ram_read_req_fifos_;
  
  // Flags indicating workload is done for each interval table controller.
  bool* done_;
};

InputReader::InputReader(char* subread_filename, uint64_t num_itcs) {
  // Read in workload parameters
  std::ifstream subread_file;
  unsigned int num_reads_ui;              // Subread file currently stores unsigned int
  unsigned int num_subreads_per_read_ui;  // TODO(Albert): Change to uint64_t
  unsigned int subread_length_ui;
  subread_file.open(subread_filename);
  subread_file.read((char *) (&num_reads_ui), sizeof(unsigned int));
  subread_file.read((char *) (&num_subreads_per_read_ui), sizeof(unsigned int));
  subread_file.read((char *) (&subread_length_ui), sizeof(unsigned int));
  num_reads_ = (uint64_t) num_reads_ui;
  num_subreads_per_read_ = (uint64_t) num_subreads_per_read_ui;
  subread_length_ = (uint64_t) subread_length_ui;
  
  // Make sure number of interval table controllers is an integer multiple of
  // the number of subreads per read
  assert(num_itcs % num_subreads_per_read_ == 0);
  num_itcs_ = num_itcs;
  
  // Make sure there is enough RAM space to store entire workload
  float num_subreads_per_ram = ceil(((float)(num_reads_ * num_subreads_per_read_)) / INPUT_READER_NUM_RAMS);
  uint64_t ram_address_width = (uint64_t) ceil(log(num_subreads_per_ram) / log(2));
  assert(INPUT_READER_RAM_ADDRESS_WIDTH >= ram_address_width);
  
  // Preload Input RAM Module with workload from given subread file
  // Workload is interleaved across RAMs to reduce bank conflicts
  input_ram_ = new RamModule<uint64_t>(INPUT_READER_NUM_RAMS, num_itcs_,
                                       INPUT_READER_RAM_ADDRESS_WIDTH, INPUT_READER_RAM_LATENCY);
  uint64_t cur_RAM = 0;
  uint64_t cur_RAM_addr_offset = 0;
  for (unsigned int i = 0; i < num_reads_; i++) {
    for (unsigned int j = 0; j < num_subreads_per_read_; j++) {
      uint64_t cur_subread;
      uint64_t cur_RAM_start_addr = cur_RAM * ((uint64_t) pow(2, INPUT_READER_RAM_ADDRESS_WIDTH));
      subread_file.read((char *) (&cur_subread), sizeof(uint64_t));
      input_ram_->WriteRequest(cur_RAM_start_addr + cur_RAM_addr_offset, 0, cur_subread);
      input_ram_->NextClockCycle();
      if (cur_RAM == INPUT_READER_NUM_RAMS - 1) {
        cur_RAM_addr_offset++;
      }
      cur_RAM = (cur_RAM + 1) % INPUT_READER_NUM_RAMS;
    }
  }
  for (unsigned int i = 0; i < INPUT_READER_RAM_LATENCY; i++) {
    input_ram_->NextClockCycle();
  }
  input_ram_->NextClockCycle();   // Extra clocks for good measure
  input_ram_->NextClockCycle();
  input_ram_->NextClockCycle();
  input_ram_->NextClockCycle();

  // Initialize workload FIFOs
  subread_fifos_ = new Fifo<SubRead>*[num_itcs_];
  ram_read_req_fifos_ = new Fifo<uint64_t>*[num_itcs_];
  for (unsigned int i = 0; i < num_itcs_; i++) {
    subread_fifos_[i] = new Fifo<SubRead>(INPUT_READER_FIFO_LENGTH);
    ram_read_req_fifos_[i] = new Fifo<uint64_t>(INPUT_READER_FIFO_LENGTH);
  }

  // Initialize other states for each interval table controller
  read_counters_ = new uint64_t[num_itcs_];
  done_ = new bool[num_itcs_];
  for (unsigned int i = 0; i < num_itcs; i++) {
    read_counters_[i] = 0;
    done_[i] = false;
  }
}

InputReader::~InputReader() {
  delete read_counters_;
  delete input_ram_;
  delete subread_fifos_;
  delete ram_read_req_fifos_;
}

SubRead InputReader::SubReadRequest(uint64_t itc_id) {
  // Make sure next subread is ready
  assert(SubReadReady(itc_id) == true);
  
  // Prepare next subread info
  SubRead sr = subread_fifos_[itc_id]->read_data();
  
  // Pop subread off of workload FIFO
  subread_fifos_[itc_id]->ReadRequest();
  
  return sr;
}

bool InputReader::SubReadReady(uint64_t itc_id) {
  return !(subread_fifos_[itc_id]->IsEmpty());
}

bool InputReader::Done() {
  for (unsigned int i = 0; i < num_itcs_; i++) {
    if (done_[i] == false || !ram_read_req_fifos_[i]->IsEmpty() || !subread_fifos_[i]->IsEmpty()) {
      return false;
    }
  }
  return true;
}

void InputReader::NextClockCycle() {
  // For each ITC FIFO, check if FIFO is full. If not, then issue a read
  // request to the input RAM for the next read.
  
  uint64_t num_parallel_reads = num_itcs_ / num_subreads_per_read_;
  for (unsigned int i = 0; i < num_itcs_; i++) {
    // When subread data is ready from RAM, pop the request off the RAM
    // read request FIFO and write the subread info to the subread FIFO.
    if (input_ram_->ReadReady(i)) {
      SubRead sr;
      sr.read_id = ram_read_req_fifos_[i]->read_data();
      sr.subread_offset = i % num_subreads_per_read_;
      sr.length = subread_length_;
      sr.data = input_ram_->ReadData(i);
      ram_read_req_fifos_[i]->ReadRequest();
      subread_fifos_[i]->WriteRequest(sr);
    }

    // If RAM is ready to accept more read requests, and there are not
    // enough outstanding requests to fill the subread FIFO, send another
    // read request.
    if (input_ram_->IsPortReady(i) &&
        ram_read_req_fifos_[i]->Size() + subread_fifos_[i]->Size() < INPUT_READER_FIFO_LENGTH) {
      // Compute next read ID
      uint64_t read_id = read_counters_[i] * num_parallel_reads + (i / num_subreads_per_read_);
      
      // Compute next subread address
      uint64_t subread_id = read_counters_[i] * num_itcs_ + i;
      uint64_t ram_id = subread_id % INPUT_READER_NUM_RAMS;
      uint64_t ram_offset = subread_id / INPUT_READER_NUM_RAMS;

      // Send requests when entire workload not completed yet
      if (read_id < num_reads_) {
        // Send next read request and record in the read request FIFO.
        ram_read_req_fifos_[i]->WriteRequest(read_id);
        input_ram_->ReadRequest(ram_id * pow(2, INPUT_READER_RAM_ADDRESS_WIDTH) + ram_offset, i);
        read_counters_[i]++;
      } else {
        done_[i] = true;
      }
    }
    
    // Clock the FIFOs
    subread_fifos_[i]->NextClockCycle();
    ram_read_req_fifos_[i]->NextClockCycle();
  }
  
  // Clock the input RAM
  input_ram_->NextClockCycle();
}

void InputReader::Reset() {
  // Reset the FIFOs
  for (unsigned int i = 0; i < num_itcs_; i++) {
    subread_fifos_[i]->Reset();
    ram_read_req_fifos_[i]->Reset();
  }
  
  // Reset the input RAM
  input_ram_->Reset();
}

#endif // CS316_CORE_INPUT_READER_H_