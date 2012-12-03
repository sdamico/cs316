// TODO(Sam): Complete implementation

#ifndef CS316_CORE_POSITION_TABLE_CTRL_H_
#define CS316_CORE_POSITION_TABLE_CTRL_H_

#include <stdint.h>
#include "sequential.h"
#include "position_table_ctrl.h"
#include "fifo.h"
#include "params.h"
#include "def.h"
#include "input_reader.h"
#include "interval_table_ctrl.h"

class PositionTableCtrl : public Sequential {
 public:
  PositionTableCtrl(uint64_t ptc_id, IntervalTableCtrl *itc, 
										RamModule<uint32_t> *position_table_ram,
										unsigned int position_table_size);
  ~PositionTableCtrl();
  void NextClockCycle();
  void Reset();
	bool PositionReady();
	PositionTableResult PositionData();
  void ReadRequest();
  
 private:
	// Compute number of position table elements for each ram
	void ComputeRamNumElem(unsigned int position_table_size);
	
	// Compute the position table ram address for a position
	uint32_t GetRamAddress(uint32_t position);

	// ID number for this position table controller.  Used as port numbers to
	// the input reader and position table ram module
	uint64_t ptc_id_;

	// Pointer to the position table ram module from which to read positions
	RamModule<uint32_t>* position_table_ram_;

	// Current Subread interval that we're operating over
	SubReadInterval sri_;

	// Position within subread interval
	uint32_t sri_offset_;

	// Position index for a given subread
	uint32_t output_position_index_;

	// Pointer to upstream ITC
	IntervalTableCtrl *itc_;

	// Fifo for storing in-flight requests from position table
  Fifo<SubReadInterval>* position_table_ram_fifo_;

	// FIFO containing subread and position info ready to be sent to stitchers
  Fifo<PositionTableResult>* output_fifo_;

	// Number of elements in the RAM
	unsigned int *ram_num_elem_;
	
};


PositionTableCtrl::PositionTableCtrl(uint64_t ptc_id,
																		 IntervalTableCtrl *itc,
																		 RamModule<uint32_t> *position_table_ram,
																		 unsigned int position_table_size) {
	ptc_id_ = ptc_id;
	itc_ = itc;
	position_table_ram_ = position_table_ram;
	ComputeRamNumElem(position_table_size);
	position_table_ram_fifo_ = new Fifo<SubReadInterval>(POSITION_TABLE_CTRL_FIFO_LENGTH);
	output_fifo_ = new Fifo<PositionTableResult>(INTERVAL_TABLE_CTRL_FIFO_LENGTH);
	output_position_index_ = 0;
	sri_offset_ = 0;
}


PositionTableCtrl::~PositionTableCtrl() {
	delete output_fifo_;
	delete position_table_ram_fifo_;
}


void PositionTableCtrl::NextClockCycle() {
	Sequential::NextClockCycle();
  if (itc_->IntervalReady() &&
      position_table_ram_fifo_->Size() + output_fifo_->Size() < POSITION_TABLE_CTRL_FIFO_LENGTH &&
      position_table_ram_->IsPortReady(ptc_id_) == true &&
			sri_offset_ == 0) {
    sri_ = itc_->IntervalData();
		
    position_table_ram_fifo_->WriteRequest(sri_);
		if(sri_.interval.length > 0) {
    	position_table_ram_->ReadRequest(GetRamAddress(sri_.interval.start), ptc_id_);
      sri_offset_ = (sri_offset_ + 1) % sri_.interval.length;
		}
  }  
  else if (position_table_ram_fifo_->Size() + output_fifo_->Size() < POSITION_TABLE_CTRL_FIFO_LENGTH && sri_offset_ > 0 && position_table_ram_->IsPortReady(ptc_id_) == true) {
    position_table_ram_fifo_->WriteRequest(sri_);		
		position_table_ram_->ReadRequest(GetRamAddress(sri_.interval.start+sri_offset_), ptc_id_);

   	sri_offset_ = (sri_offset_ + 1) % sri_.interval.length;
	}


	// Process the lookup when it is ready from the position table ram module.
	// If it's the last lookup for a given subread, pop the subread info off
	// the position table RAM fifo
	SubReadInterval sri = position_table_ram_fifo_->read_data();
	
	if (!position_table_ram_fifo_->IsEmpty() &&
      (position_table_ram_fifo_->read_data().interval.length == 0)) {
    position_table_ram_fifo_->ReadRequest();
		PositionTableResult ptr;
		ptr.sr = sri.sr;
		ptr.position = 0;
		ptr.last = true;
		ptr.empty = true;
		output_fifo_->WriteRequest(ptr);
	}
  else if (position_table_ram_->ReadReady(ptc_id_)) {
    uint32_t position = position_table_ram_->ReadData(ptc_id_);
    position_table_ram_fifo_->ReadRequest();

    bool last_position = false;
    if (output_position_index_ == (sri.interval.length-1)) {
			// Tells stitcher this is the last position for this subread
			last_position = true;
			output_position_index_ = 0;
    } else {
			output_position_index_++;
		}

    // Send position if valid (i.e. positive position-offset)
    if (sri.sr.length * sri.sr.subread_offset <= position) {
      PositionTableResult ptr;
      ptr.sr = sri.sr;
      ptr.position = position;
      ptr.last = last_position;
      ptr.empty = false;
  /*		std::cout<<"out "<<ptc_id_<<" read id: "<<ptr.sr.read_id<<std::endl;
      std::cout<<"out "<<ptc_id_<<" read offset: "<<ptr.sr.subread_offset<<std::endl;

      std::cout<<"out "<<ptc_id_<<" position: "<<ptr.position<<std::endl;
      std::cout<<"out "<<ptr.last<<std::endl;*/
      output_fifo_->WriteRequest(ptr);
    } else {
      // If no valid positions (i.e. positive position-offset), send an empty position
      if (last_position == true) {
        PositionTableResult ptr;
        ptr.sr = sri.sr;
        ptr.position = 0;
        ptr.last = true;
        ptr.empty = true;
        output_fifo_->WriteRequest(ptr);
      }
    }
  }


	position_table_ram_fifo_->NextClockCycle();
	output_fifo_->NextClockCycle();
}

void PositionTableCtrl::Reset() {
	output_fifo_->Reset();
	position_table_ram_fifo_->Reset();
}

bool PositionTableCtrl::PositionReady() {
  return !output_fifo_->IsEmpty();
}

PositionTableResult PositionTableCtrl::PositionData() {
  PositionTableResult result = output_fifo_->read_data();
  return result;
}

void PositionTableCtrl::ReadRequest() {
  output_fifo_->ReadRequest();
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
