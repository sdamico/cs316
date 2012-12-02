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

#define NUM_RAMS 8
#define RAM_ADDRESS_WIDTH 8
#define RAM_ADDRESS_WIDTH_PTC 8
#define RAM_LATENCY 2

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
  unsigned int interval_table_ram_size = NUM_RAMS * pow(2, RAM_ADDRESS_WIDTH);
  uint32_t* interval_table_ram_array = new uint32_t[interval_table_ram_size];
  for (unsigned int i = 0; i < interval_table_ram_size; i++) {
    interval_table_ram_array[i] = 0;
  }
  unsigned int* ram_num_elem = new unsigned int[NUM_RAMS];
  for (unsigned int i = 0; i < NUM_RAMS; i++) {
    ram_num_elem[i] = interval_table_size / NUM_RAMS;
  }
  for (unsigned int i = 0; i < interval_table_size % NUM_RAMS; i++) {
    ram_num_elem[i]++;
  }
  unsigned int ram_id = 0;
  unsigned int ram_addr = 0;
  for (unsigned int i = 0; i < interval_table_size; i++) {
    interval_table_ram_array[(ram_id << RAM_ADDRESS_WIDTH) + ram_addr] = interval_table[i];
    if (ram_addr == ram_num_elem[ram_id] - 1) {
      ram_id++;
      ram_addr = 0;
    } else {
      ram_addr++;
    }
  }
  interval_table_ram = new RamModule<uint32_t>(NUM_RAMS, num_itcs, RAM_ADDRESS_WIDTH, RAM_LATENCY);
  interval_table_ram->Preload(interval_table_ram_array, interval_table_ram_size, false);

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
  unsigned int position_table_ram_size = NUM_RAMS * pow(2, RAM_ADDRESS_WIDTH_PTC);
  uint32_t* position_table_ram_array = new uint32_t[position_table_ram_size];
  for (unsigned int i = 0; i < position_table_ram_size; i++) {
    position_table_ram_array[i] = 0;
  }
  unsigned int* ram_num_elem_ptc = new unsigned int[NUM_RAMS];
  for (unsigned int i = 0; i < NUM_RAMS; i++) {
    ram_num_elem_ptc[i] = position_table_size / NUM_RAMS;
  }
  for (unsigned int i = 0; i < position_table_size % NUM_RAMS; i++) {
    ram_num_elem_ptc[i]++;
  }
  ram_id = 0;
  ram_addr = 0;
  for (unsigned int i = 0; i < position_table_size; i++) {
    position_table_ram_array[(ram_id << RAM_ADDRESS_WIDTH_PTC) + ram_addr] = position_table[i];
    if (ram_addr == ram_num_elem_ptc[ram_id] - 1) {
      ram_id++;
      ram_addr = 0;
    } else {
      ram_addr++;
    }
  }
  position_table_ram = new RamModule<uint32_t>(NUM_RAMS, num_ptcs, RAM_ADDRESS_WIDTH_PTC, RAM_LATENCY);
  position_table_ram->Preload(position_table_ram_array, position_table_ram_size, false);

  
  // Instantiate test position table controllers
  
  // Instantiate interval table controllers
  itcs = new IntervalTableCtrl*[num_itcs];
  for (unsigned int i = 0; i < num_itcs; i++) {
    itcs[i] = new IntervalTableCtrl(i, input_reader, interval_table_ram, interval_table_size);
  }
  
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
         
        // Check subread information

        //assert(ptr.sr.read_id == read_counters[i] * num_parallel_reads + i / num_subreads_per_read);
				//std::cout<<"read id: "<<ptr.sr.read_id<<", offset: "<<ptr.sr.subread_offset<<", position: "<<ptr.position<<", last: "<<ptr.last<<std::endl;
        read_counters[i]++;
        
				//std::cout<<"ptc: "<<i<<std::endl;
				//interval_table[sri.sr.data]
        // Check interval information
        /*std::cout<<"offset: "<<ptc_offsets[i]<<std::endl;
        std::cout<<"predicted interval: "<<interval_table[ptr.sr.data]<<std::endl;
				std::cout<<"predicted position: "<<position_table[interval_table[ptr.sr.data] + ptc_offsets[i]]<<std::endl;
        std::cout<<"position: "<<ptr.position<<std::endl;*/
				assert(ptr.position == position_table[interval_table[ptr.sr.data] + ptc_offsets[i]]);
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
  for (unsigned int i = 0; i < NUM_RAMS; i++) {
    std::cout<<"RAM "<<i<<": "<<interval_table_ram->GetAccessCount(i)<<std::endl;
  }
  std::cout << "Interval Table Controller tests complete!" << std::endl;
  return 0;
}
