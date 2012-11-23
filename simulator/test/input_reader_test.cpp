// Test assumes INPUT_READER_NUM_RAMS == 8
//
// Test uses a subread sequence file generated from a reference length of 100, query length
// of 20, and subread length of 5 to test the case where num_itcs == INPUT_READER_NUM_RAMS
// so that there are no input ram bank conflicts
//
// Test uses a subread sequence file generated from a reference length of 100, query length
// of 20, and a subread length of 4 to test the case where num_itcs != INPUT_READER_NUM_RAMS
// so that there are input ram bank conflicts

#include <iostream>
#include <fstream>
#include "input_reader.h"
#include <assert.h>
#include <list>

int main (int argc, char** argv) {
  if (argc < 3) {
    std::cout << "Usage: " << argv[0] << " <100.20.5 Subread Filename> <100.20.4 Subread Filename" << std::endl;
    exit(1);
  }
  
  //------------------------- num_itcs == INPUT_READER_NUM_RAMS test -----------------------
  
  // Read the subread parameters from the file
  std::ifstream subread_file;
  unsigned int num_reads;
  unsigned int num_subreads_per_read;
  unsigned int subread_length;
  subread_file.open(argv[1]);
  subread_file.read((char *) (&num_reads), sizeof(unsigned int ));
  subread_file.read((char *) (&num_subreads_per_read), sizeof(unsigned int ));
  subread_file.read((char *) (&subread_length), sizeof(unsigned int ));
  unsigned int num_itcs = num_subreads_per_read * 2;
  unsigned int num_parallel_reads = num_itcs / num_subreads_per_read;
  assert(num_itcs % num_subreads_per_read == 0);
  
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
  InputReader* ir = new InputReader(argv[1], num_itcs);
  uint64_t start_cycle = ir->cycle_count();
  
  // Wait for RAM latency clock cycles for subread FIFO to start
  // filling up with subread work units
  // Note: 3 extra clocks of latency:
  //     1 clock to go into the RAM Module input FIFO
  //     1 clock to go into the RAM Module output FIFO
  //     1 clock to go into the Input Reader subread FIFO
  for (unsigned int i = 0; i < INPUT_READER_RAM_LATENCY + 3; i++) {
    for (unsigned int j = 0; j < num_itcs; j++) {
      assert(ir->Done() == false);
      assert(ir->SubReadReady(j) == false);
    }
    ir->NextClockCycle();
  }

  // Check first subread 
  for (unsigned int i = 0; i < num_itcs; i++) {
    assert(ir->Done() == false);
    assert(ir->SubReadReady(i) == true);
    SubRead sr = ir->SubReadRequest(i);
    assert(sr.read_id == i / num_subreads_per_read);
    assert(sr.subread_offset == i % num_subreads_per_read);
    assert(sr.length == subread_length);
    assert(sr.data == subread_list[i].front());
    subread_list[i].pop();
  }
  ir->NextClockCycle();

  // Get every even subread
  for (unsigned int i = 0; i < num_itcs; i+=2) {
    assert(ir->Done() == false);
    assert(ir->SubReadReady(i) == true);
    SubRead sr = ir->SubReadRequest(i);
    assert(sr.read_id == num_parallel_reads + i / num_subreads_per_read);
    assert(sr.subread_offset == i % num_subreads_per_read);
    assert(sr.length == subread_length);
    assert(sr.data == subread_list[i].front());
    subread_list[i].pop();
  }
  ir->NextClockCycle();
  
  // Get all subreads again
  for (unsigned int i = 0; i < num_itcs; i++) {
    assert(ir->Done() == false);
    assert(ir->SubReadReady(i) == true);
    SubRead sr = ir->SubReadRequest(i);
    if (i % 2 == 0) {
      assert(sr.read_id == 2 * num_parallel_reads + i / num_subreads_per_read);
    } else {
      assert(sr.read_id == 1 * num_parallel_reads + i / num_subreads_per_read);
    }
    assert(sr.subread_offset == i % num_subreads_per_read);
    assert(sr.length == subread_length);
    assert(sr.data == subread_list[i].front());
    subread_list[i].pop();
  }
  ir->NextClockCycle();
  
  // Get every odd subread
  for (unsigned int i = 1; i < num_itcs; i+=2) {
    assert(ir->Done() == false);
    assert(ir->SubReadReady(i) == true);
    SubRead sr = ir->SubReadRequest(i);
    assert(sr.read_id == 2 * num_parallel_reads + i / num_subreads_per_read);
    assert(sr.subread_offset == i % num_subreads_per_read);
    assert(sr.length == subread_length);
    assert(sr.data == subread_list[i].front());
    subread_list[i].pop();
  }
  ir->NextClockCycle();
  
  // Finish getting other subreads
  for (unsigned int i = 0; i < 37; i++) {
    for (unsigned int j = 0; j < num_itcs; j++) {
      assert(ir->Done() == false);
      assert(ir->SubReadReady(j) == true);
      SubRead sr = ir->SubReadRequest(j);
      assert(sr.read_id == (i+3) * num_parallel_reads + j / num_subreads_per_read);
      assert(sr.subread_offset == j % num_subreads_per_read);
      assert(sr.length == subread_length);
      assert(sr.data == subread_list[j].front());
      subread_list[j].pop();
    }
    ir->NextClockCycle();
  }
  for (unsigned int i = 0; i < 4; i++) {
    assert(ir->Done() == false);
    assert(ir->SubReadReady(i) == true);
    SubRead sr = ir->SubReadRequest(i);
    assert(sr.read_id == 40 * num_parallel_reads + i / num_subreads_per_read);
    assert(sr.subread_offset == i % num_subreads_per_read);
    assert(sr.length == subread_length);
    assert(sr.data == subread_list[i].front());
    subread_list[i].pop();
  }
  ir->NextClockCycle();
  
  // Check that workload is done
  assert(ir->Done() == true);
  std::cout << "No conflict test took " << ir->cycle_count() - start_cycle << " cycles" << std::endl;
  //------------------------- num_itcs != INPUT_READER_NUM_RAMS test -----------------------
  // Read the subread parameters from the file
  subread_file.open(argv[2]);
  subread_file.read((char *) (&num_reads), sizeof(unsigned int ));
  subread_file.read((char *) (&num_subreads_per_read), sizeof(unsigned int));
  subread_file.read((char *) (&subread_length), sizeof(unsigned int));

  num_itcs = num_subreads_per_read * 2;
  num_parallel_reads = num_itcs / num_subreads_per_read;

  // Store the subreads into lists to be used later
  subread_list = new std::queue<uint64_t>[num_itcs];
  cur_subread = 0;
  while (cur_subread < num_reads * num_subreads_per_read) {
    uint64_t subread;
    unsigned int cur_itc = cur_subread % num_itcs;
    subread_file.read((char *) (&subread), sizeof(uint64_t));
    subread_list[cur_itc].push(subread);
    cur_subread++;
  }
  subread_file.close();

  // Instantiate the input reader
  ir = new InputReader(argv[2], num_itcs);
  start_cycle = ir->cycle_count();
  
  // Initialize read counters for individual itcs
  uint64_t* read_counters = new uint64_t[num_itcs];
  for(unsigned int i = 0; i < num_itcs; i++) {
    read_counters[i] = 0;
  }
  
  // Keep pulling work out of input reader until done
  while (!ir->Done()) {
    for (unsigned int i = 0; i < num_itcs; i++) {
      if (ir->SubReadReady(i) == true) {
        SubRead sr = ir->SubReadRequest(i);
        assert(sr.read_id == read_counters[i] * num_parallel_reads + i / num_subreads_per_read);
        assert(sr.subread_offset == i % num_subreads_per_read);
        assert(sr.length == subread_length);
        assert(sr.data == subread_list[i].front());
        subread_list[i].pop();
        read_counters[i]++;
      }
    }
    ir->NextClockCycle();
  }
  
  // Check that workload is done
  assert(ir->Done() == true);
  std::cout << "Conflict test took " << ir->cycle_count() - start_cycle << " cycles" << std::endl;
  
  std::cout << "Input Reader tests complete!" << std::endl;
  return 0;
}