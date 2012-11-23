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
  unsigned int ram_size = (unsigned int) pow(2, RAM_ADDRESS_WIDTH);
  unsigned int ram_module_size = NUM_RAMS * ram_size;
  int* test_values = new int[ram_module_size];
  for (unsigned int i = 0; i < ram_module_size; i++) {
    test_values[i] = i;
  }
  
  // Reset and preload RAM module
  rm.Reset();
  rm.Preload(test_values, ram_module_size, false);
  
  // Check initial state
  for (int i = 0; i < NUM_PORTS; i++) {
    assert(rm.IsPortReady(i) == true);
    assert(rm.ReadReady(i) == false);
  }
  assert(rm.Size() == ram_module_size);
  
  // Single port non-interleaving full readout test
  for (unsigned int i = 0; i < ram_module_size; i++) {
    rm.ReadRequest(i, 0);
    if (i >= RAM_LATENCY + 2) {
      unsigned int test_addr = i - (RAM_LATENCY + 2);
      assert(rm.ReadReady(0) == true);
      assert(rm.ReadData(0) == (int) (test_addr));
    }
    rm.NextClockCycle();
  }
  for (unsigned int i = ram_module_size - (RAM_LATENCY + 2); i < ram_size; i++) {
    assert(rm.ReadReady(0) == true);
    assert(rm.ReadData(0) == (int) i);
    rm.NextClockCycle();
  }
  
  // Single port interleaving full readout test
  rm.Reset();
  rm.Preload(test_values, ram_module_size, true);
  for (unsigned int i = 0; i < ram_module_size; i++) {
    rm.ReadRequest(i, 0);
    if (i >= RAM_LATENCY + 2) {
      unsigned int test_addr = i - (RAM_LATENCY + 2);
      assert(rm.ReadReady(0) == true);
      assert(rm.ReadData(0) == (int) ((test_addr % ram_size) * NUM_RAMS + (test_addr / ram_size)));
    }
    rm.NextClockCycle();
  }
  for (unsigned int i = ram_module_size - (RAM_LATENCY + 2); i < ram_module_size; i++) {
    assert(rm.ReadReady(0) == true);
    assert(rm.ReadData(0) == (int) ((i % ram_size) * NUM_RAMS + (i / ram_size)));
    rm.NextClockCycle();
  }
  
  // Multi-port simultaneous read to separate banks test
  rm.Reset();
  rm.Preload(test_values, ram_module_size, false);
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
  rm.Preload(test_values, ram_module_size, false);
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
  rm.Preload(test_values, ram_module_size, false);
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
  rm.Preload(test_values, ram_module_size, false);
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