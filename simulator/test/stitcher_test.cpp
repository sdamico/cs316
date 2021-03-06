#include <iostream>
#include <fstream>
#include "stitcher.h"
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
Stitcher** stitchers;

unsigned int num_itcs;
unsigned int num_ptcs;
unsigned int num_stitchers;

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
  for (unsigned int i = 0; i < num_stitchers; i++) {
    stitchers[i]->NextClockCycle();
  }
}

int main (int argc, char** argv) {
  if (argc < 4) {
    std::cout << "Usage: " << argv[0] << " <Subread Filename> <Interval Table Filename> <Position Table Filename> <Results Filename>" << std::endl;
    exit(1);
  }
  
  // Read the subread parameters from the file
  std::cout<<"Reading subreads"<<std::endl;
  std::ifstream subread_file;
  unsigned int num_reads;
  unsigned int num_subreads_per_read;
  unsigned int subread_length;
  subread_file.open(argv[1]);
  subread_file.read((char *) (&num_reads), sizeof(unsigned int ));
  subread_file.read((char *) (&num_subreads_per_read), sizeof(unsigned int ));
  subread_file.read((char *) (&subread_length), sizeof(unsigned int ));
  num_stitchers = 2;
  num_itcs = num_subreads_per_read * num_stitchers;
	num_ptcs = num_itcs;
  
  // Store the subreads
  uint64_t** subreads = new uint64_t*[num_reads];
  for (unsigned int i = 0; i < num_reads; i++) {
    subreads[i] = new uint64_t[num_subreads_per_read];
  }
  for (unsigned int i = 0; i < num_reads; i++) {
    for (unsigned int j = 0; j < num_subreads_per_read; j++) {
      uint64_t subread;
      subread_file.read((char *)(&subread), sizeof(uint64_t));
      subreads[i][j] = subread;
    }
  }
  subread_file.close();
  
  // Instantiate the input reader
  input_reader = new InputReader(argv[1], num_itcs);
  
  // Build the interval table
  std::cout<<"Building interval table"<<std::endl;
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
  
  // Read in the results file
  // NOTE: ASSUMES ONE POSITION PER READ
  // TODO: Remove this assumption
  std::cout<<"Reading results file"<<std::endl;
  std::ifstream results_file(argv[4]);
  char buffer[256];
  results_file.getline(buffer, 256);
  unsigned int num_results = atoi(buffer);
  unsigned int* results = new unsigned int[num_results];
  for (unsigned int i = 0; i < num_results; i++) {
    results_file.getline(buffer, 256);
    results[i] = atoi(buffer);
  }
  results_file.close();
  
  // Instantiate interval table controllers
  std::cout<<"Instantiating ITCs"<<std::endl;
  itcs = new IntervalTableCtrl*[num_itcs];
  for (unsigned int i = 0; i < num_itcs; i++) {
    itcs[i] = new IntervalTableCtrl(i, input_reader, interval_table_ram, interval_table_size);
  }
  
  // Instantiate position table controllers
  std::cout<<"Instantiating PTCs"<<std::endl;
	ptcs = new PositionTableCtrl*[num_ptcs];
	for (unsigned int i = 0; i < num_ptcs; i++) {
		ptcs[i] = new PositionTableCtrl(i, itcs[i], position_table_ram, position_table_size);
	}

  // Instantiate stitchers
  std::cout<<"Instantiating Stitchers"<<std::endl;
  stitchers = new Stitcher*[num_stitchers];
  for (unsigned int i = 0; i < num_stitchers; i++) {
    stitchers[i] = new Stitcher(num_ptcs/num_stitchers, &(ptcs[i*num_ptcs/num_stitchers]));
  }
  
  // Perform tests
  std::cout<<"Performing tests"<<std::endl;
  uint64_t start_cycle = input_reader->cycle_count();
  unsigned int num_results_checked = 0;
  while (num_results_checked < num_results) {
    for (unsigned int i = 0; i < num_stitchers; i++) {
      if (stitchers[i]->ReadPositionReady() == true) {
        ReadPosition rp = stitchers[i]->ReadPositionData();
        stitchers[i]->ReadRequest();
        assert(results[rp.read_id] == rp.position);
        num_results_checked++;
        if (num_results_checked % 1000 == 0) {
          std::cout << num_results_checked << " out of " << num_results << std::endl;
        }
      }
    }
    Clock();
  }
  assert(input_reader->Done());
  
  std::cout << "Test took " << input_reader->cycle_count() - start_cycle << " cycles" << std::endl;
  std::cout << "Stitcher tests complete!" << std::endl;
  return 0;
}
