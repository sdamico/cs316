// TODO(Albert): Complete implementation

#ifndef CS316_CORE_STITCHER_H_
#define CS316_CORE_STITCHER_H_

#include <stdint.h>
#include "sequential.h"
#include "def.h"
#include "position_table_ctrl.h"
#include "fifo.h"

class Stitcher : public Sequential {
 public:
  Stitcher(uint64_t num_ptcs, PositionTableCtrl** ptcs);
  ~Stitcher();
  void NextClockCycle();
  void Reset();  
  
  bool ReadPositionReady();
  ReadPosition ReadPositionData();
  void ReadRequest();
  
 private:
  enum State {
    STITCHING,
    FLUSHING
  };
  
  State FlushIteration();
  
  uint64_t num_ptcs_;
  PositionTableCtrl** ptcs_;

  State state;
  Fifo<ReadPosition>* output_fifo_;
};

Stitcher::Stitcher(uint64_t num_ptcs, PositionTableCtrl** ptcs) {
  num_ptcs_ = num_ptcs;
  ptcs_ = ptcs;
  state = STITCHING;
  output_fifo_ = new Fifo<ReadPosition>(STITCHER_OUTPUT_FIFO_LENGTH);
}

Stitcher::~Stitcher() {
  delete output_fifo_;
}

bool Stitcher::ReadPositionReady() {
  return !(output_fifo_->IsEmpty());
}

ReadPosition Stitcher::ReadPositionData() {
  ReadPosition rp = output_fifo_->read_data();
  return rp;
}

void Stitcher::ReadRequest() {
  output_fifo_->ReadRequest();
}

void Stitcher::Reset() {
  output_fifo_->Reset();
  state = STITCHING;
}

Stitcher::State Stitcher::FlushIteration() {
  // Check if all PTCs are on the last element for the query
  bool all_last = true;
  for (unsigned int i = 0; i < num_ptcs_; i++) {
    if ((ptcs_[i]->PositionData()).last == false) {
      all_last = false;
      break;
    }
  }
  
  // All PTCs are flushed - pop once and revert to STITCHING state
  State next_state;
  if (all_last == true) {
    for (unsigned int i = 0; i < num_ptcs_; i++) {
      ptcs_[i]->ReadRequest();
    }
    next_state = STITCHING;
  }
  // Not all PTCs are flushed - continuing flushing the non-flushed PTCs
  else {
    for (unsigned int i = 0; i < num_ptcs_; i++) {
      if ((ptcs_[i]->PositionData()).last == false) {
        ptcs_[i]->ReadRequest();
      }
    }
    next_state = FLUSHING;
  }
  
  return next_state;
}

// NOTE: Assumes that positions coming from the PTC is positive after taking into
//       account subread offsets
void Stitcher::NextClockCycle() {
  Sequential::NextClockCycle();
  
  // Check to see if all PTCs are ready with a value
  bool all_valid = true;
  for (unsigned int i = 0; i < num_ptcs_; i++) {
    if (!(ptcs_[i]->PositionReady())) {
      all_valid = false;
      break;
    }
  }
  
  if (all_valid == true) {
    switch (state) {
      // Currently finding position matches
      case STITCHING: {
        // Check if any position lists are empty
        bool any_empty = false;
        for (unsigned int i = 0; i < num_ptcs_; i++) {
          if ((ptcs_[i]->PositionData()).empty == true) {
            any_empty = true;
            break;
          }
        }
        // Start flushing if any position list is empty
        if (any_empty == true) {
          state = FlushIteration();
          break;
        }
        
        // Check if all PTC positions match
        bool all_match = true;
        for (unsigned int i = 0; i < num_ptcs_-1; i++) {
          PositionTableResult ptc1 = ptcs_[i]->PositionData();
          PositionTableResult ptc2 = ptcs_[i+1]->PositionData();
          uint64_t ptc1_sr_offset = ptc1.sr.subread_offset * ptc1.sr.length;
          uint64_t ptc2_sr_offset = ptc2.sr.subread_offset * ptc2.sr.length;
          
          if ((ptc1.position - ptc1_sr_offset) != (ptc2.position - ptc2_sr_offset)) {
            all_match = false;
            break;
          }
        }
        
        // All positions match - store position in FIFO and pop from all PTCs
        if (all_match == true) {
          ReadPosition rp;
          rp.read_id = (ptcs_[0]->PositionData()).sr.read_id;
          rp.position = (ptcs_[0]->PositionData()).position;
          output_fifo_->WriteRequest(rp);
          bool started_flushing = false;
          for (unsigned int i = 0 ; i < num_ptcs_; i++) {
            if ((ptcs_[i]->PositionData()).last == true) {
              state = FlushIteration();
              started_flushing = true;
              break;
            } else {
              ptcs_[i]->ReadRequest();
            }
          }
          if (started_flushing == true) {
            break;
          }
        }
        // Not all positions match - find max PTC position, and pop from all PTCs
        //   whose positions are less than the max.
        else {
          // Find max PTC value
          uint64_t max_ptc_value = 0;
          for (unsigned int i = 0; i < num_ptcs_; i++) {
            PositionTableResult ptc = ptcs_[i]->PositionData();
            uint64_t ptc_sr_offset = ptc.sr.subread_offset * ptc.sr.length;
            
            if ((ptc.position - ptc_sr_offset) > max_ptc_value) {
              max_ptc_value = ptc.position - ptc_sr_offset;
            }
          }
          
          // Pop all PTCs with less than max
          bool started_flushing = false;
          for (unsigned int i = 0; i < num_ptcs_; i++) {
            PositionTableResult ptc = ptcs_[i]->PositionData();
            uint64_t ptc_sr_offset = ptc.sr.subread_offset * ptc.sr.length;
            
            if (ptc.position - ptc_sr_offset < max_ptc_value) {
              if (ptc.last == true) {
                state = FlushIteration();
                started_flushing = true;
                break;
              } else {
                ptcs_[i]->ReadRequest();
              }
            }
          }
          if (started_flushing == true) {
            break;
          }
        }
        break;
      }
      
      // Currently flushing PTCs to synchronize to the same query
      case FLUSHING: {
        state = FlushIteration();
        break;
      }
        
      // Should never get here
      default: break;
    }
  }
  
  // Clock all sub-elements
  output_fifo_->NextClockCycle();
}

#endif // CS316_CORE_STITCHER_H_
