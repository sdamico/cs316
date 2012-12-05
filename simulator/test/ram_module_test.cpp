#include <iostream>
#include "ram_module.h"
#include <assert.h>
#include <cmath>

#define NUM_RAMS 8
#define NUM_PORTS 8
#define ADDR_ROW_WIDTH 2
#define ADDR_COL_WIDTH 2
#define ADDR_BANK_WIDTH 2
#define RAM_ADDRESS_WIDTH (ADDR_ROW_WIDTH+ADDR_COL_WIDTH+ADDR_BANK_WIDTH)

#define TRCD 16
#define TCL 14
#define TRP 16
#define SYSTEM_CLK_FREQ 400
#define MEM_CLK_FREQ 400

int main (int argc, char** argv) {
  RamModule<unsigned int> rm(NUM_RAMS, NUM_PORTS, ADDR_ROW_WIDTH, ADDR_COL_WIDTH,
                    ADDR_BANK_WIDTH, SYSTEM_CLK_FREQ, MEM_CLK_FREQ,
                    TRCD, TCL, TRP);
  unsigned int ram_size = (unsigned int) pow(2, RAM_ADDRESS_WIDTH);
  unsigned int ram_module_size = NUM_RAMS * ram_size;
  unsigned int* test_values = new unsigned int[ram_module_size];
  for (unsigned int i = 0; i < ram_module_size; i++) {
    test_values[i] = i;
  }
  
  // Reset and preload RAM module
  rm.Reset();
  rm.Preload(test_values, ram_module_size);
  
  // Check initial state
  for (int i = 0; i < NUM_PORTS; i++) {
    assert(rm.IsPortReady(i) == true);
    assert(rm.ReadReady(i) == false);
  }
  assert(rm.Size() == ram_module_size);
  
  // Single port full readout test
  unsigned int test_addr = 0;
  for (unsigned int i = 0; i < ram_module_size; ) {
    if (rm.IsPortReady(0) == true) {
      rm.ReadRequest(i, 0);
      i++;
    }
    if (rm.ReadReady(0) == true) {
      assert(rm.ReadData(0) == test_addr);
      test_addr++;
    }
    rm.NextClockCycle();
  }
  while (test_addr < ram_module_size) {
    if (rm.ReadReady(0) == true) {
      assert(rm.ReadData(0) == test_addr);
      test_addr++;
    }
    rm.NextClockCycle();
  }
  
  // Multi-port simultaneous read to separate RAMs test
  rm.Reset();
  rm.Preload(test_values, ram_module_size);
  for (unsigned int i = 0; i < NUM_PORTS; i++) {
    rm.ReadRequest(i << RAM_ADDRESS_WIDTH, i);
  }
  while (rm.ReadReady(0) == false) {
    rm.NextClockCycle();
  }
  for (unsigned int i = 0; i < NUM_PORTS; i++) {
    assert(rm.ReadReady(i) == true);
    assert(rm.ReadData(i) == i << RAM_ADDRESS_WIDTH);
  }
  
  // Two ports read conflict test
  rm.Reset();
  rm.Preload(test_values, ram_module_size);
  rm.ReadRequest(0, 0);
  rm.ReadRequest(1, 1);
  while (rm.ReadReady(0) == false) {
    assert(rm.ReadReady(1) == false);
    rm.NextClockCycle();
  }
  assert(rm.ReadData(0) == 0);
  assert(rm.ReadReady(1) == false);
  while (rm.ReadReady(1) == false) {
    rm.NextClockCycle();
  }
  assert(rm.ReadData(1) == 1);
  
  // Non-starvation test
  rm.Reset();
  rm.Preload(test_values, ram_module_size);
  rm.ReadRequest(0, 0);
  rm.ReadRequest(1, 1);
  unsigned int port = 2;
  bool still_sending = true;
  while (rm.ReadReady(0) == false) {
    for (unsigned int i = 0; i < NUM_PORTS; i++) {
      assert(rm.ReadReady(i) == false);
    }
    rm.ReadRequest(0, 0);
    if (still_sending == true) {
      rm.ReadRequest(port, port);
      if (port == NUM_PORTS - 1) {
        still_sending = false;
      } else {
        port++;
      }
    }
    rm.NextClockCycle();
  }
  assert(rm.ReadData(0) == 0);
  rm.NextClockCycle();
  unsigned int read_port = 1;
  while (read_port < NUM_PORTS) {
    while (rm.ReadReady(read_port) == false) {
      for (unsigned int i = 0; i < NUM_PORTS; i++) {
        assert(rm.ReadReady(i) == false);
      }
      if (still_sending == true) {
        rm.ReadRequest(port, port);
        if (port == NUM_PORTS - 1) {
          still_sending = false;
        } else {
          port++;
        }
      }
      rm.NextClockCycle();
    }
    assert(rm.ReadData(read_port) == read_port);
    read_port++;
    rm.NextClockCycle();
  }
  
  std::cout << "RAM Module tests complete!" << std::endl;
  return 0;
}