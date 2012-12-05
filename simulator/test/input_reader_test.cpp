#include <iostream>
#include <fstream>
#include "input_reader.h"
#include <assert.h>
#include <list>
#include <stdlib.h>
#include "def.h"

int main (int argc, char** argv) {
  if (argc < 2) {
    std::cout << "Usage: " << argv[0] << " <Subread Filename>" << std::endl;
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
  unsigned int num_itcs = num_subreads_per_read * 2;
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
  
  uint64_t num_subreads_read = 0;
  while (num_subreads_read < num_reads * num_subreads_per_read) {
    for (unsigned int i = 0; i < num_itcs; i++) {
      if (ir->SubReadReady(i)) {
        SubRead sr = ir->SubReadRequest(i);
        std::cout<<sr.data <<" "<<subread_list[i].front()<<std::endl;
        //assert(sr.data == subread_list[i].front());
        subread_list[i].pop();
        num_subreads_read++;
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
