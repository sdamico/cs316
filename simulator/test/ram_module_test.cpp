#include <iostream>
#include "ram_module.h"
#include <assert.h>
#include <cmath>

#define NUM_RAMS 8
#define NUM_PORTS 8
#define RAM_ADDRESS_WIDTH 5
#define RAM_LATENCY 2

int main (int argc, char** argv) {
  RamModule<int> rm(NUM_RAMS, NUM_PORTS, RAM_ADDRESS_WIDTH, RAM_LATENCY);
  Ram<int>** rams = rm.GetRams();
  
  // Preload RAMs with incrementing values
  for (int i = 0; i < NUM_RAMS; i++) {
    for (int j = 0; j < (int) pow(2, RAM_ADDRESS_WIDTH); j++) {
      rams[i]->WriteRequest(j, (i << RAM_ADDRESS_WIDTH) + j);
      rams[i]->NextClockCycle();
      rams[i]->NextClockCycle();
    }
  }
  
  // Check initial state
  for (int i = 0; i < NUM_PORTS; i++) {
    assert(rm.IsPortReady(i) == true);
    assert(rm.ReadReady(i) == false);
  }
  
  // Multi-port simultaneous read to separate banks
  for (int i = 0; i < NUM_PORTS; i++) {
    rm.ReadRequest(i << RAM_ADDRESS_WIDTH, i);
  }
  rm.NextClockCycle();
  for (int i = 0; i < NUM_PORTS; i++) {
    assert(rm.ReadReady(i) == false);
  }
  rm.NextClockCycle();
  for (int i = 0; i < NUM_PORTS; i++) {
    assert(rm.ReadReady(i) == false);
  }
  rm.NextClockCycle();
  for (int i = 0; i < NUM_PORTS; i++) {
    assert(rm.ReadReady(i) == false);
  }
  rm.NextClockCycle();
  for (int i = 0; i < NUM_PORTS; i++) {
    std::cout<<rm.ReadReady(i) << std::endl;
    assert(rm.ReadReady(i));
    assert(rm.ReadData(i) == i);
  }

  
  std::cout << "RAM Module tests complete!" << std::endl;
  return 0;
}