// File Name         : params.h
// Description       : Changeable parameters to measure performance impact
// Table of Contents : input_reader.h parameters
// Revision History  :
//     Albert Ng      Nov 19 2012     Initial Revision

#ifndef CS316_CORE_PARAMS_H_
#define CS316_CORE_PARAMS_H_

// input_reader.h
#define INPUT_READER_FIFO_LENGTH 64
#define INPUT_READER_NUM_RAMS 8
#define INPUT_READER_RAM_ADDRESS_WIDTH 21
#define INPUT_READER_RAM_LATENCY 2

// interval_table_ctrl.h
#define INTERVAL_TABLE_CTRL_FIFO_LENGTH 16

// position_table_ctrl.h
#define POSITION_TABLE_CTRL_FIFO_LENGTH 16

// stitcher.h
#define STITCHER_OUTPUT_FIFO_LENGTH 128

#endif // CS316_CORE_PARAMS_H_
