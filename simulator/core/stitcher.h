// TODO(Albert): Complete implementation

#ifndef CS316_CORE_STITCHER_H_
#define CS316_CORE_STITCHER_H_

#include <cstdint>
#include "sequential.h"

class Stitcher : public Sequential {
 public:
  Stitcher();
  ~Stitcher();
  void NextClockCycle();
  void Reset();  

  // Returns true if Stitcher is ready to receive a new position
  bool ReceiveReady();
  
  // Receive and store the interval info for processing
  void ReceivePosition(SubReadInterval sri);

	// Current read that this stitcher is operating over
	uint64_t read_id();

	bool done_stitching();
  
 private:

	uint64_t read_id_;

	bool done_stitching_;
};

#endif // CS316_CORE_STITCHER_H_
