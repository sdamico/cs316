// File Name         : params.h
// Description       : Changeable parameters to measure performance impact
// Table of Contents : input_reader.h parameters
// Revision History  :
//     Albert Ng      Nov 19 2012     Initial Revision

#ifndef CS316_CORE_PARAMS_H_
#define CS316_CORE_PARAMS_H_

#include <stdint.h>
#include <cmath>

// input_reader.h
#define INPUT_READER_TOTAL_ADDR_WIDTH 20
uint64_t INPUT_READER_FIFO_LENGTH = 64;
uint64_t INPUT_READER_NUM_RAMS = 8;
#define INPUT_READER_RAM_ADDR_ROW_WIDTH (INPUT_READER_TOTAL_ADDR_WIDTH - INPUT_READER_RAM_ADDR_BANK_WIDTH - INPUT_READER_RAM_ADDR_COL_WIDTH - (uint64_t) (log((float) INPUT_READER_NUM_RAMS) / log(2)))
uint64_t INPUT_READER_RAM_ADDR_COL_WIDTH = 7;
uint64_t INPUT_READER_RAM_ADDR_BANK_WIDTH = 3;
uint64_t INPUT_READER_RAM_ADDR_WIDTH = (INPUT_READER_RAM_ADDR_BANK_WIDTH + INPUT_READER_RAM_ADDR_ROW_WIDTH + INPUT_READER_RAM_ADDR_COL_WIDTH);
uint64_t INPUT_READER_SYSTEM_CLOCK_FREQ_MHZ = 200;
uint64_t INPUT_READER_MEMORY_CLOCK_FREQ_MHZ = 400;
uint64_t INPUT_READER_RAM_TRCD = 8;
uint64_t INPUT_READER_RAM_TCL = 7;
uint64_t INPUT_READER_RAM_TRP = 8;

// interval_table_ctrl.h
#define INTERVAL_TABLE_CTRL_TOTAL_ADDR_WIDTH 21
uint64_t INTERVAL_TABLE_CTRL_FIFO_LENGTH = 64;
uint64_t INTERVAL_TABLE_CTRL_NUM_RAMS = 8;
#define INTERVAL_TABLE_CTRL_RAM_ADDR_ROW_WIDTH (INTERVAL_TABLE_CTRL_TOTAL_ADDR_WIDTH - INTERVAL_TABLE_CTRL_RAM_ADDR_BANK_WIDTH - INTERVAL_TABLE_CTRL_RAM_ADDR_COL_WIDTH - (uint64_t) (log((float) INTERVAL_TABLE_CTRL_NUM_RAMS) / log(2)))
uint64_t INTERVAL_TABLE_CTRL_RAM_ADDR_COL_WIDTH = 7;
uint64_t INTERVAL_TABLE_CTRL_RAM_ADDR_BANK_WIDTH = 3;
uint64_t INTERVAL_TABLE_CTRL_RAM_ADDR_WIDTH = (INTERVAL_TABLE_CTRL_RAM_ADDR_BANK_WIDTH + INTERVAL_TABLE_CTRL_RAM_ADDR_ROW_WIDTH + INTERVAL_TABLE_CTRL_RAM_ADDR_COL_WIDTH);
uint64_t INTERVAL_TABLE_CTRL_SYSTEM_CLOCK_FREQ_MHZ = 200;
uint64_t INTERVAL_TABLE_CTRL_MEMORY_CLOCK_FREQ_MHZ = 400;
uint64_t INTERVAL_TABLE_CTRL_RAM_TRCD = 8;
uint64_t INTERVAL_TABLE_CTRL_RAM_TCL = 7;
uint64_t INTERVAL_TABLE_CTRL_RAM_TRP = 8;
uint64_t num_skipped_subreads = 0;  // TODO: Make this not global

// position_table_ctrl.h
#define POSITION_TABLE_CTRL_TOTAL_ADDR_WIDTH 28
uint64_t POSITION_TABLE_CTRL_FIFO_LENGTH = 64;
uint64_t POSITION_TABLE_CTRL_NUM_RAMS = 8;
#define POSITION_TABLE_CTRL_RAM_ADDR_ROW_WIDTH (POSITION_TABLE_CTRL_TOTAL_ADDR_WIDTH - POSITION_TABLE_CTRL_RAM_ADDR_BANK_WIDTH - POSITION_TABLE_CTRL_RAM_ADDR_COL_WIDTH - (uint64_t) (log((float) POSITION_TABLE_CTRL_NUM_RAMS) / log(2)))
uint64_t POSITION_TABLE_CTRL_RAM_ADDR_COL_WIDTH = 7;
uint64_t POSITION_TABLE_CTRL_RAM_ADDR_BANK_WIDTH = 3;
uint64_t POSITION_TABLE_CTRL_RAM_ADDR_WIDTH = (POSITION_TABLE_CTRL_RAM_ADDR_BANK_WIDTH + POSITION_TABLE_CTRL_RAM_ADDR_ROW_WIDTH + POSITION_TABLE_CTRL_RAM_ADDR_COL_WIDTH);
uint64_t POSITION_TABLE_CTRL_SYSTEM_CLOCK_FREQ_MHZ = 200;
uint64_t POSITION_TABLE_CTRL_MEMORY_CLOCK_FREQ_MHZ = 400;
uint64_t POSITION_TABLE_CTRL_RAM_TRCD = 8;
uint64_t POSITION_TABLE_CTRL_RAM_TCL = 7;
uint64_t POSITION_TABLE_CTRL_RAM_TRP = 8;


// ram_module.h
uint64_t RAM_MODULE_PORT_INPUT_FIFO_LENGTH = 64;
uint64_t RAM_MODULE_PORT_ROB_SIZE = 64;
uint64_t RAM_MODULE_INFLIGHT_FIFO_LENGTH = 64;

// stitcher.h
uint64_t STITCHER_OUTPUT_FIFO_LENGTH = 128;

#endif // CS316_CORE_PARAMS_H_
