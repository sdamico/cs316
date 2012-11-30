// TODO(Sam): Complete implementation

#ifndef CS316_CORE_POSITION_TABLE_CTRL_H_
#define CS316_CORE_POSITION_TABLE_CTRL_H_

#include <stdint.h>
#include "sequential.h"
#include "def.h"

class PositionTableCtrl : public Sequential {
 public:
  PositionTableCtrl(uint64_t ptc_id, RamModule<uint32_t> *position_table_ram,
										Stitcher *stitchers, uint32_t num_stitchers,
										position_table_size);
  ~PositionTableCtrl();
  void NextClockCycle();
  void Reset();
  
  // Position Table Controller interface function stubs
    
 private:
	// Pointer to all stitchers
	Stitcher *stitchers_;

	// Number of stitchers
	uint32_t num_stitchers_;

	// Current stitcher being used
	uint32_t cur_stitcher_;

	// Compute number of position table elements for each ram
	void ComputeRamNumElem(unsigned int position_table_size);
	
	// Compute the position table ram address for a position
	uint64_t GetRamAddress(uint32_t position);

	// ID number for this position table controller.  Used as port numbers to
	// the input reader and position table ram module
	uint64_t ptc_id_;

	// Pointer to the position table ram module from which to read positions
	RamModule<uint32_t>* position_table_ram_;

	// Current Subread interval that we're operating over
	SubReadInterval sri_;

	// Position within subread interval
	uint32_t sri_offset_;

	// Fifo for storing in-flight requests from position table
  Fifo<SubRead>* position_table_ram_fifo_;

	// FIFO containing subread and position info ready to be sent to stitchers
  Fifo<PositionTableResult>* output_fifo_;
	
};


PositionTableCtrl::PositionTableCtrl(uint64_t ptc_id,
																		 RamModule<uint64_t> *position_table_ram,
																		 Stitcher *stitchers,
																		 uint32_t num_stitchers,
																		 position_table_size) {
	ptc_id_ = ptc_id;
	position_table_ram_ = position_table_ram;
	stitchers_ = stitchers;
	num_stitchers_ = num_stitchers;
	ComputeRamNumElem(position_table_size);
}


PositionTableCtrl::~PositionTableCtrl() {
}


void PositionTableCtrl::NextClockCycle() {
	Sequential::NextClockCycle();

  if (itc_->interval_ready() &&
      position_table_ram_fifo_->Size() + output_fifo_->Size() < POSITION_TABLE_CTRL_FIFO_LENGTH &&
      position_table_ram_->IsPortReady(ptc_id_) == true &&
			sri_offset_ == 0) {
    sri_ = itc_->interval();

    position_table_ram_fifo_->WriteRequest(sri);
    position_table_ram_->ReadRequest(GetRamAddress(sri_.sr.start), ptc_id_);
   	sri_offset_++; 
  }  
  if (sri_offset_ > 0 && position_table_ram_->IsPortReady(ptc_id_) == true) {
		if(sri_offset_ == sri_.sr.length) {
			sri_offset_ == 0;
		}
		position_table_ram_->ReadRequest(GetRamAddress(sri_.sr.start+sri_offset_), itc_id_);
  	sri_offset_++;
	}


	// Process the lookup when it is ready from the position table ram module.
	// If it's the last lookup for a given subread, pop the subread info off
	// the position table RAM fifo
  if (position_table_ram_->ReadReady(ptc_id_)) {
    uint64_t lookup_data = position_table_ram_->ReadData(ptc_id_);
		bool last_position = false;
    if (sri_offset_ == (sri_.interval.length-1)) {
			// Tells stitcher this is the last position for this subread
			last_position = true;
    } 

    uint64_t position = position_table_ram_fifo_->read_data();
    position_table_ram_fifo_->ReadRequest();
      
    PositionTableResult ptr;
	  ptr.sr = sr;
	  ptr.position = position;
		ptr.last = last_position;
    
		output_fifo_->WriteRequest(ptr);
  }


	position_table_ram_fifo_->NextClockCycle();
	output_fifo_->NextClockCycle();
}

void PositionTableCtrl::Reset() {
}

void PositionTableCtrl::ComputeRamNumElem(unsigned int position_table_size) {
  unsigned int num_rams = position_table_ram_->num_rams();
  ram_num_elem_ = new unsigned int[num_rams];
  for (unsigned int i = 0; i < num_rams; i++) {
    ram_num_elem_[i] = position_table_size / num_rams;
  }
  for (unsigned int i = 0; i < position_table_size % num_rams; i++) {
    ram_num_elem_[i]++;
  }
}
                                     
uint32_t PositionTableCtrl::GetRamAddress(uint32_t position) {
  unsigned int ram_id = 0;
  while (position >= ram_num_elem_[ram_id]) {
    position -= ram_num_elem_[ram_id];
    ram_id++;
  }
  return ((ram_id << position_table_ram_->ram_address_width()) + position);
}


#endif // CS316_CORE_POSITION_TABLE_CTRL_H_
