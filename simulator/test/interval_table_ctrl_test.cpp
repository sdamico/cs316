#include <iostream>
#include <fstream>
#include "position_table_ctrl.h"
#include "interval_table_ctrl.h"
#include "input_reader.h"
#include "def.h"
#include <assert.h>
#include <list>
#include <stdint.h>
#include <cmath>
#include <stdlib.h>
#include "params.h"


InputReader* input_reader;
RamModule<uint32_t>* interval_table_ram;
IntervalTableCtrl** itcs;
PositionTableCtrl** ptcs;
unsigned int num_itcs;

void Clock() {
  input_reader->NextClockCycle();
  interval_table_ram->NextClockCycle();
  for (unsigned int i = 0; i < num_itcs; i++) {
    itcs[i]->NextClockCycle();
  }
}

int main (int argc, char** argv) {
  if (argc < 3) {
    std::cout << "Usage: " << argv[0] << " <Subread Filename> <Interval Table Filename>" << std::endl;
    exit(1);
  }
  
  // Read the subread parameters from the file
  std::ifstream subread_file;
  unsigned int num_reads;
  unsigned int num_subreads_per_read;
  unsigned int subread_length;
  subread_file.open(argv[1]);
  subread_file.read((char *) (&num_reads), sizeof(unsigned int ));
  subread_file.read((char *) (&num_subreads_per_read), sizeof(unsigned int ));
  subread_file.read((char *) (&subread_length), sizeof(unsigned int ));
  num_itcs = num_subreads_per_read * 2;
  unsigned int num_parallel_reads = num_itcs / num_subreads_per_read;
  
  // Store the subreads into lists to be used later
  std::queue<uint64_t>* subread_list = new std::queue<uint64_t>[num_itcs];
  unsigned int cur_subread = 0;
  while (cur_subread < num_reads * num_subreads_per_read) {
    uint64_t subread;
    unsigned int cur_itc = cur_subread % num_itcs;
    subread_file.read((char *) (&subread), sizeof(uint64_t));
    subread_list[cur_itc].push(subread);
    cur_subread++;
  }
  subread_file.close();
  
  // Instantiate the input reader
  input_reader = new InputReader(argv[1], num_itcs);
  
  // Build the interval table
  unsigned int interval_table_size;
  std::ifstream interval_table_file;
  interval_table_file.open(argv[2]);
  interval_table_file.read((char *)(&interval_table_size), sizeof(unsigned int));
  uint32_t* interval_table = new uint32_t[interval_table_size];
  interval_table_file.read((char *)interval_table, interval_table_size * sizeof(uint32_t));
  interval_table_file.close();
  
  // Build the RAM module preload array
  unsigned int interval_table_ram_size = INTERVAL_TABLE_CTRL_NUM_RAMS * pow(2, INTERVAL_TABLE_CTRL_RAM_ADDR_WIDTH);
  uint32_t* interval_table_ram_array = new uint32_t[interval_table_ram_size];
  
  for (unsigned int i = 0; i < interval_table_ram_size; i++) {
    interval_table_ram_array[i] = 0;
  }
  unsigned int** ram_bank_num_elem = new unsigned int*[INTERVAL_TABLE_CTRL_NUM_RAMS];
  for (unsigned int i = 0; i < INTERVAL_TABLE_CTRL_NUM_RAMS; i++) {
    ram_bank_num_elem[i] = new unsigned int[(unsigned int) pow(2, INTERVAL_TABLE_CTRL_RAM_ADDR_BANK_WIDTH)];
    for (unsigned int j = 0; j < pow(2, INTERVAL_TABLE_CTRL_RAM_ADDR_BANK_WIDTH); j++) {
      ram_bank_num_elem[i][j] = interval_table_size / INTERVAL_TABLE_CTRL_NUM_RAMS / ((unsigned int) pow(2, INTERVAL_TABLE_CTRL_RAM_ADDR_BANK_WIDTH));
    }
  }
  for (unsigned int i = 0; i < interval_table_size % (INTERVAL_TABLE_CTRL_NUM_RAMS * ((unsigned int) pow(2, INTERVAL_TABLE_CTRL_RAM_ADDR_BANK_WIDTH))); i++) {
    ram_bank_num_elem[i / ((unsigned int) pow(2, INTERVAL_TABLE_CTRL_RAM_ADDR_BANK_WIDTH))][i % ((unsigned int) pow(2, INTERVAL_TABLE_CTRL_RAM_ADDR_BANK_WIDTH))]++;
  }
  unsigned int ram_id = 0;
  unsigned int bank_id = 0;
  unsigned int ram_addr = 0;
  for (unsigned int i = 0; i < interval_table_size; i++) {
    interval_table_ram_array[(ram_id << INTERVAL_TABLE_CTRL_RAM_ADDR_WIDTH) + (bank_id << (INTERVAL_TABLE_CTRL_RAM_ADDR_ROW_WIDTH + INTERVAL_TABLE_CTRL_RAM_ADDR_COL_WIDTH)) + ram_addr] = interval_table[i];
    if (ram_addr == ram_bank_num_elem[ram_id][bank_id] - 1) {
      bank_id++;
      ram_addr = 0;
      if (bank_id == pow(2, INTERVAL_TABLE_CTRL_RAM_ADDR_BANK_WIDTH)) {
        ram_id++;
        bank_id = 0;
      }
    } else {
      ram_addr++;
    }
  }
  interval_table_ram = new RamModule<uint32_t>(INTERVAL_TABLE_CTRL_NUM_RAMS, num_itcs, INTERVAL_TABLE_CTRL_RAM_ADDR_ROW_WIDTH,
                                               INTERVAL_TABLE_CTRL_RAM_ADDR_COL_WIDTH, INTERVAL_TABLE_CTRL_RAM_ADDR_BANK_WIDTH,
                                               INTERVAL_TABLE_CTRL_SYSTEM_CLOCK_FREQ_MHZ, INTERVAL_TABLE_CTRL_MEMORY_CLOCK_FREQ_MHZ,
                                               INTERVAL_TABLE_CTRL_RAM_TRCD, INTERVAL_TABLE_CTRL_RAM_TCL,
                                               INTERVAL_TABLE_CTRL_RAM_TRP);
  interval_table_ram->Preload(interval_table_ram_array, interval_table_ram_size);
  
  // Instantiate interval table controllers
  itcs = new IntervalTableCtrl*[num_itcs];
  for (unsigned int i = 0; i < num_itcs; i++) {
    itcs[i] = new IntervalTableCtrl(i, input_reader, interval_table_ram, interval_table_size);
  }

  uint64_t* read_counters = new uint64_t[num_itcs];
  for (unsigned int i = 0; i < num_itcs; i++) {
    read_counters[i] = 0;
  }
  uint64_t start_cycle = input_reader->cycle_count();
  bool done = false;
  do {
    done = input_reader->Done();
    for (unsigned int i = 0; i < num_itcs; i++) {
      if (itcs[i]->IntervalReady() == true) {
        SubReadInterval sri = itcs[i]->IntervalData();
        
        // Check subread information
        assert(sri.sr.read_id == read_counters[i] * num_parallel_reads + i / num_subreads_per_read);
        assert(sri.sr.subread_offset == i % num_subreads_per_read);
        assert(sri.sr.length == subread_length);
        assert(sri.sr.data == subread_list[i].front());
        subread_list[i].pop();
        read_counters[i]++;
        
        // Check interval information
        assert(sri.interval.start == interval_table[sri.sr.data]);
        assert(sri.interval.length == interval_table[sri.sr.data + 1] - interval_table[sri.sr.data]);
        
        done = !(!done || !(itcs[i]->IsIdle()));
      }
    }
    Clock();
  } while (!done);

  std::cout << "Test took " << input_reader->cycle_count() - start_cycle << " cycles" << std::endl;
  for (unsigned int i = 0; i < INTERVAL_TABLE_CTRL_NUM_RAMS; i++) {
    std::cout<<"RAM "<<i<<": "<<interval_table_ram->GetAccessCount(i)<<std::endl;
  }
  std::cout << "Interval Table Controller tests complete!" << std::endl;
  return 0;
}
