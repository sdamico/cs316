#include <iostream>
#include "ram_module.h"
#include <assert.h>
#include <cmath>

#define NUM_RAMS 8
#define NUM_PORTS 8
#define RAM_ADDRESS_WIDTH 5
#define RAM_LATENCY 2

// Helper function to preload RAMs with incrementing values
void preload (RamModule<int>* rm) {
  Ram<int>** rams = rm->rams();
  
  for (int i = 0; i < NUM_RAMS; i++) {
    for (int j = 0; j < (int) pow(2, RAM_ADDRESS_WIDTH); j++) {
      rams[i]->WriteRequest(j, (i << RAM_ADDRESS_WIDTH) + j);
      rams[i]->NextClockCycle();
      rams[i]->NextClockCycle();
    }
  }
}

int main (int argc, char** argv) {
  RamModule<int> rm(NUM_RAMS, NUM_PORTS, RAM_ADDRESS_WIDTH, RAM_LATENCY);
  
  // Reset and preload RAM module
  rm.Reset();
  preload(&rm);
  
  // Check initial state
  for (int i = 0; i < NUM_PORTS; i++) {
    assert(rm.IsPortReady(i) == true);
    assert(rm.ReadReady(i) == false);
  }
  assert(rm.Size() == NUM_RAMS * pow(2, RAM_ADDRESS_WIDTH));
  
  // Multi-port simultaneous read to separate banks test
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
    assert(rm.ReadReady(i) == true);
    assert(rm.ReadData(i) == (i << RAM_ADDRESS_WIDTH));
  }

  // Two ports read conflict test
  rm.Reset();
  preload(&rm);
  rm.ReadRequest(0, 0);
  rm.ReadRequest(1, 1);
  rm.NextClockCycle();
  assert(rm.ReadReady(0) == false);
  assert(rm.ReadReady(1) == false);
  rm.NextClockCycle();
  assert(rm.ReadReady(0) == false);
  assert(rm.ReadReady(1) == false);
  rm.NextClockCycle();
  assert(rm.ReadReady(0) == false);
  assert(rm.ReadReady(1) == false);
  rm.NextClockCycle();
  assert(rm.ReadReady(0) == true);
  assert(rm.ReadReady(1) == false);
  assert(rm.ReadData(0) == 0);
  rm.NextClockCycle();
  assert(rm.ReadReady(0) == false);
  assert(rm.ReadReady(1) == true);
  assert(rm.ReadData(1) == 1);
  rm.NextClockCycle();
  
  // Non-starvation test
  rm.Reset();
  preload(&rm);
  rm.ReadRequest(0, 0);
  rm.ReadRequest(1, 1);
  rm.NextClockCycle();
  assert(rm.ReadReady(0) == false);
  assert(rm.ReadReady(1) == false);
  rm.ReadRequest(0, 0);
  rm.ReadRequest(2, 2);
  rm.NextClockCycle();
  assert(rm.ReadReady(0) == false);
  assert(rm.ReadReady(1) == false);
  assert(rm.ReadReady(2) == false);
  rm.ReadRequest(0, 0);
  rm.NextClockCycle();
  assert(rm.ReadReady(0) == false);
  assert(rm.ReadReady(1) == false);
  assert(rm.ReadReady(2) == false);
  rm.ReadRequest(0, 0);
  rm.NextClockCycle();
  assert(rm.ReadReady(0) == true);
  assert(rm.ReadReady(1) == false);
  assert(rm.ReadReady(2) == false);
  assert(rm.ReadData(0) == 0);
  rm.ReadRequest(0, 0);
  rm.NextClockCycle();
  assert(rm.ReadReady(0) == false);
  assert(rm.ReadReady(1) == true);
  assert(rm.ReadReady(2) == false);
  assert(rm.ReadData(1) == 1);
  rm.ReadRequest(0, 0);
  rm.NextClockCycle();
  assert(rm.ReadReady(0) == false);
  assert(rm.ReadReady(1) == false);
  assert(rm.ReadReady(2) == true);
  assert(rm.ReadData(2) == 2);
  rm.NextClockCycle();
  assert(rm.ReadReady(0) == true);
  assert(rm.ReadReady(1) == false);
  assert(rm.ReadReady(2) == false);
  assert(rm.ReadData(0) == 0);
  rm.NextClockCycle();
  assert(rm.ReadReady(0) == true);
  assert(rm.ReadReady(1) == false);
  assert(rm.ReadReady(2) == false);
  assert(rm.ReadData(0) == 0);
  rm.NextClockCycle();
  assert(rm.ReadReady(0) == true);
  assert(rm.ReadReady(1) == false);
  assert(rm.ReadReady(2) == false);
  assert(rm.ReadData(0) == 0);
  rm.NextClockCycle();
  assert(rm.ReadReady(0) == true);
  assert(rm.ReadReady(1) == false);
  assert(rm.ReadReady(2) == false);
  assert(rm.ReadData(0) == 0);
  rm.NextClockCycle();
  assert(rm.ReadReady(0) == true);
  assert(rm.ReadReady(1) == false);
  assert(rm.ReadReady(2) == false);
  assert(rm.ReadData(0) == 0);

  // Write then read test
  rm.Reset();
  preload(&rm);
  rm.ReadRequest(0, 0);
  rm.NextClockCycle();
  rm.WriteRequest(0, 0, (int) 0xDEADBEEF);
  assert(rm.ReadReady(0) == false);
  rm.NextClockCycle();
  rm.ReadRequest(0, 0);
  assert(rm.ReadReady(0) == false);
  rm.NextClockCycle();
  assert(rm.ReadReady(0) == false);
  rm.NextClockCycle();
  assert(rm.ReadReady(0) == true);
  assert(rm.ReadData(0) == 0);
  rm.NextClockCycle();
  assert(rm.ReadReady(0) == false);
  rm.NextClockCycle();
  assert(rm.ReadReady(0) == true);
  assert(rm.ReadData(0) == (int) 0xDEADBEEF);

  
  std::cout << "RAM Module tests complete!" << std::endl;
  return 0;
}