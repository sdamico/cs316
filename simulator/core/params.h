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
#define INPUT_READER_RAM_ADDR_ROW_WIDTH 5
#define INPUT_READER_RAM_ADDR_COL_WIDTH 5
#define INPUT_READER_RAM_ADDR_BANK_WIDTH 3
#define INPUT_READER_RAM_ADDR_WIDTH (INPUT_READER_RAM_ADDR_BANK_WIDTH + INPUT_READER_RAM_ADDR_ROW_WIDTH + INPUT_READER_RAM_ADDR_COL_WIDTH)
#define INPUT_READER_SYSTEM_CLOCK_FREQ_MHZ 200
#define INPUT_READER_MEMORY_CLOCK_FREQ_MHZ 400
#define INPUT_READER_RAM_TRCD 8
#define INPUT_READER_RAM_TCL 7
#define INPUT_READER_RAM_TRP 8

// interval_table_ctrl.h
#define INTERVAL_TABLE_CTRL_FIFO_LENGTH 16

// position_table_ctrl.h
#define POSITION_TABLE_CTRL_FIFO_LENGTH 16

// ram_module.h
#define RAM_MODULE_PORT_INPUT_FIFO_LENGTH 64
#define RAM_MODULE_PORT_ROB_SIZE 64
#define RAM_MODULE_INFLIGHT_FIFO_LENGTH 64

#endif // CS316_CORE_PARAMS_H_
