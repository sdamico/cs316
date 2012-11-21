// File Name         : def.h
// Description       : General definitions used throughout the system
// Table of Contents :
// Revision History  :
//     Albert Ng      Nov 20 2012     Initial Revision

#ifndef CS316_CORE_DEF_H_
#define CS316_CORE_DEF_H_

#include <stdint.h>

// Subread identification information and data
struct SubRead {
  uint64_t read_id;
  uint64_t subread_offset;
  uint64_t length;
  uint64_t data;
};

// Position table interval information for a subread
struct PositionTableInterval{
  uint32_t start;
  uint32_t length;
};

#endif // CS316_CORE_DEF_H_