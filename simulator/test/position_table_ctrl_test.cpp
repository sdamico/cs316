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
RamModule<uint32_t>* position_table_ram;
PositionTableCtrl** ptcs;

unsigned int num_itcs;
unsigned int num_ptcs;

void Clock() {
  input_reader->NextClockCycle();
  interval_table_ram->NextClockCycle();
  position_table_ram->NextClockCycle();
  for (unsigned int i = 0; i < num_ptcs; i++) {
    ptcs[i]->NextClockCycle();
  }
  for (unsigned int i = 0; i < num_itcs; i++) {
    itcs[i]->NextClockCycle();
  }
}

int main (int argc, char** argv) {
  if (argc < 4) {
    std::cout << "Usage: " << argv[0] << " <Subread Filename> <Interval Table Filename> <Position Table Filename>" << std::endl;
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
  num_ptcs = num_itcs;
  
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
  unsigned int** interval_ram_bank_num_elem = new unsigned int*[INTERVAL_TABLE_CTRL_NUM_RAMS];
  for (unsigned int i = 0; i < INTERVAL_TABLE_CTRL_NUM_RAMS; i++) {
    interval_ram_bank_num_elem[i] = new unsigned int[(unsigned int) pow(2, INTERVAL_TABLE_CTRL_RAM_ADDR_BANK_WIDTH)];
    for (unsigned int j = 0; j < pow(2, INTERVAL_TABLE_CTRL_RAM_ADDR_BANK_WIDTH); j++) {
      interval_ram_bank_num_elem[i][j] = interval_table_size / INTERVAL_TABLE_CTRL_NUM_RAMS / ((unsigned int) pow(2, INTERVAL_TABLE_CTRL_RAM_ADDR_BANK_WIDTH));
    }
  }
  for (unsigned int i = 0; i < interval_table_size % (INTERVAL_TABLE_CTRL_NUM_RAMS * ((unsigned int) pow(2, INTERVAL_TABLE_CTRL_RAM_ADDR_BANK_WIDTH))); i++) {
    interval_ram_bank_num_elem[i / ((unsigned int) pow(2, INTERVAL_TABLE_CTRL_RAM_ADDR_BANK_WIDTH))][i % ((unsigned int) pow(2, INTERVAL_TABLE_CTRL_RAM_ADDR_BANK_WIDTH))]++;
  }
  unsigned int interval_ram_id = 0;
  unsigned int interval_bank_id = 0;
  unsigned int interval_ram_addr = 0;
  for (unsigned int i = 0; i < interval_table_size; i++) {
    interval_table_ram_array[(interval_ram_id << INTERVAL_TABLE_CTRL_RAM_ADDR_WIDTH) + (interval_bank_id << (INTERVAL_TABLE_CTRL_RAM_ADDR_ROW_WIDTH + INTERVAL_TABLE_CTRL_RAM_ADDR_COL_WIDTH)) + interval_ram_addr] = interval_table[i];
    if (interval_ram_addr == interval_ram_bank_num_elem[interval_ram_id][interval_bank_id] - 1) {
      interval_bank_id++;
      interval_ram_addr = 0;
      if (interval_bank_id == pow(2, INTERVAL_TABLE_CTRL_RAM_ADDR_BANK_WIDTH)) {
        interval_ram_id++;
        interval_bank_id = 0;
      }
    } else {
      interval_ram_addr++;
    }
  }
  interval_table_ram = new RamModule<uint32_t>(INTERVAL_TABLE_CTRL_NUM_RAMS, num_itcs, INTERVAL_TABLE_CTRL_RAM_ADDR_ROW_WIDTH,
                                               INTERVAL_TABLE_CTRL_RAM_ADDR_COL_WIDTH, INTERVAL_TABLE_CTRL_RAM_ADDR_BANK_WIDTH,
                                               INTERVAL_TABLE_CTRL_SYSTEM_CLOCK_FREQ_MHZ, INTERVAL_TABLE_CTRL_MEMORY_CLOCK_FREQ_MHZ,
                                               INTERVAL_TABLE_CTRL_RAM_TRCD, INTERVAL_TABLE_CTRL_RAM_TCL,
                                               INTERVAL_TABLE_CTRL_RAM_TRP);
  interval_table_ram->Preload(interval_table_ram_array, interval_table_ram_size);

  // Build the position table
  unsigned int position_table_size;
  unsigned int ref_seq_length;
  unsigned int seed_length;
  std::ifstream position_table_file;
  position_table_file.open(argv[3]);
  position_table_file.read((char *)(&ref_seq_length), sizeof(unsigned int));
  position_table_file.read((char *)(&seed_length), sizeof(unsigned int));
  uint32_t *position_table = new uint32_t[ref_seq_length - seed_length + 1];
  position_table_size = ref_seq_length - seed_length + 1;
  position_table_file.read((char *)(position_table), (ref_seq_length - seed_length + 1) * sizeof(unsigned int));
  position_table_file.close();
 
  // Build the RAM module preload array
  unsigned int position_table_ram_size = POSITION_TABLE_CTRL_NUM_RAMS * pow(2, POSITION_TABLE_CTRL_RAM_ADDR_WIDTH);
  uint32_t* position_table_ram_array = new uint32_t[position_table_ram_size];
  for (unsigned int i = 0; i < position_table_ram_size; i++) {
    position_table_ram_array[i] = 0;
  }
  unsigned int** position_ram_bank_num_elem = new unsigned int*[POSITION_TABLE_CTRL_NUM_RAMS];
  for (unsigned int i = 0; i < POSITION_TABLE_CTRL_NUM_RAMS; i++) {
    position_ram_bank_num_elem[i] = new unsigned int[(unsigned int) pow(2, POSITION_TABLE_CTRL_RAM_ADDR_BANK_WIDTH)];
    for (unsigned int j = 0; j < pow(2, POSITION_TABLE_CTRL_RAM_ADDR_BANK_WIDTH); j++) {
      position_ram_bank_num_elem[i][j] = position_table_size / POSITION_TABLE_CTRL_NUM_RAMS / ((unsigned int) pow(2, POSITION_TABLE_CTRL_RAM_ADDR_BANK_WIDTH));
    }
  }
  for (unsigned int i = 0; i < position_table_size % (POSITION_TABLE_CTRL_NUM_RAMS * ((unsigned int) pow(2, POSITION_TABLE_CTRL_RAM_ADDR_BANK_WIDTH))); i++) {
    position_ram_bank_num_elem[i / ((unsigned int) pow(2, POSITION_TABLE_CTRL_RAM_ADDR_BANK_WIDTH))][i % ((unsigned int) pow(2, POSITION_TABLE_CTRL_RAM_ADDR_BANK_WIDTH))]++;
  }
  unsigned int position_ram_id = 0;
  unsigned int position_bank_id = 0;
  unsigned int position_ram_addr = 0;
  for (unsigned int i = 0; i < position_table_size; i++) {
    position_table_ram_array[(position_ram_id << POSITION_TABLE_CTRL_RAM_ADDR_WIDTH) + (position_bank_id << (POSITION_TABLE_CTRL_RAM_ADDR_ROW_WIDTH + POSITION_TABLE_CTRL_RAM_ADDR_COL_WIDTH)) + position_ram_addr] = position_table[i];
    if (position_ram_addr == position_ram_bank_num_elem[position_ram_id][position_bank_id] - 1) {
      position_bank_id++;
      position_ram_addr = 0;
      if (position_bank_id == pow(2, POSITION_TABLE_CTRL_RAM_ADDR_BANK_WIDTH)) {
        position_ram_id++;
        position_bank_id = 0;
      }
    } else {
      position_ram_addr++;
    }
  }
  position_table_ram = new RamModule<uint32_t>(POSITION_TABLE_CTRL_NUM_RAMS, num_itcs, POSITION_TABLE_CTRL_RAM_ADDR_ROW_WIDTH,
                                               POSITION_TABLE_CTRL_RAM_ADDR_COL_WIDTH, POSITION_TABLE_CTRL_RAM_ADDR_BANK_WIDTH,
                                               POSITION_TABLE_CTRL_SYSTEM_CLOCK_FREQ_MHZ, POSITION_TABLE_CTRL_MEMORY_CLOCK_FREQ_MHZ,
                                               POSITION_TABLE_CTRL_RAM_TRCD, POSITION_TABLE_CTRL_RAM_TCL,
                                               POSITION_TABLE_CTRL_RAM_TRP);
  position_table_ram->Preload(position_table_ram_array, position_table_ram_size);

  // Instantiate interval table controllers
  itcs = new IntervalTableCtrl*[num_itcs];
  for (unsigned int i = 0; i < num_itcs; i++) {
    itcs[i] = new IntervalTableCtrl(i, input_reader, interval_table_ram, interval_table_size);
  }
  
  // Instantiate test position table controllers
  ptcs = new PositionTableCtrl*[num_ptcs];
  for (unsigned int i = 0; i < num_ptcs; i++) {
    ptcs[i] = new PositionTableCtrl(i, itcs[i], position_table_ram, position_table_size);
  }

  uint64_t* read_counters = new uint64_t[num_itcs];
  for (unsigned int i = 0; i < num_itcs; i++) {
    read_counters[i] = 0;
  }
  uint64_t start_cycle = input_reader->cycle_count();
  
  unsigned int *ptc_offsets = (unsigned int*)calloc(num_ptcs, sizeof(unsigned int));


  while (!input_reader->Done()) {
    for (unsigned int i = 0; i < num_ptcs; i++) {
      if (ptcs[i]->PositionReady() == true) {
        PositionTableResult ptr = ptcs[i]->PositionData();
        ptcs[i]->ReadRequest();
         
        // Check subread information
        read_counters[i]++;
        
        //std::cout<<"ptc: "<<i<<std::endl;
        //interval_table[sri.sr.data]
        // Check interval information
        /*std::cout<<"offset: "<<ptc_offsets[i]<<std::endl;
        std::cout<<"predicted interval: "<<interval_table[ptr.sr.data]<<std::endl;
        std::cout<<"predicted position: "<<position_table[interval_table[ptr.sr.data] + ptc_offsets[i]]<<std::endl;
        std::cout<<"position: "<<ptr.position<<std::endl;*/
        if (!(ptr.empty)) {
          //std::cout<<ptr.position<<" "<<position_table[interval_table[ptr.sr.data] + ptc_offsets[i]]<<std::endl;
          while (position_table[interval_table[ptr.sr.data] + ptc_offsets[i]] < ptr.sr.length * ptr.sr.subread_offset) {
            ptc_offsets[i]++;
          }
          assert(ptr.position == position_table[interval_table[ptr.sr.data] + ptc_offsets[i]]);
        }
        assert(ptc_offsets[i] < (interval_table[ptr.sr.data+1] - interval_table[ptr.sr.data]));
        //assert(sri.interval.length == interval_table[sri.sr.data + 1] - interval_table[sri.sr.data]);
        if(ptr.last) {
          assert(ptc_offsets[i] == (interval_table[ptr.sr.data+1] - interval_table[ptr.sr.data] - 1));
          ptc_offsets[i] = 0;
        }
        else {
          ptc_offsets[i]++;
        }
      }
    }
    Clock();
  }
  
  std::cout << "Test took " << input_reader->cycle_count() - start_cycle << " cycles" << std::endl;
  //for (unsigned int i = 0; i < NUM_RAMS; i++) {
  //  std::cout<<"RAM "<<i<<": "<<interval_table_ram->GetAccessCount(i)<<std::endl;
  //}
  std::cout << "Position Table Controller tests complete!" << std::endl;
  return 0;
}
