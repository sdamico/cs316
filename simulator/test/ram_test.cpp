#include <iostream>
#include "ram.h"
#include <assert.h>
#include <cmath>

#define ADDRESS_WIDTH 5
#define LATENCY 2

int main (int arg, char** argv) {
  Ram<int> r(ADDRESS_WIDTH, LATENCY);
  
  // Check initial states
  assert(r.read_ready() == false);
  
  // Read after Write Test
  r.WriteRequest(0, (int) 0xDEADBEEF);
  r.NextClockCycle();
  assert(r.read_ready() == false);
  r.ReadRequest(0);
  r.NextClockCycle();
  assert(r.read_ready() == false);
  r.NextClockCycle();
  assert(r.read_ready() == true);
  assert(r.read_data() == (int) 0xDEADBEEF);
  r.NextClockCycle();
  assert(r.read_ready() == false);
  
  // Multiple Reads after Writes Test
  r.WriteRequest(1, (int) 0xFEEDCAFE);
  r.NextClockCycle();
  assert(r.read_ready() == false);
  r.WriteRequest(1, (int) 0xFEEDBEEF);
  r.NextClockCycle();
  assert(r.read_ready() == false);
  r.ReadRequest(1);
  r.NextClockCycle();
  assert(r.read_ready() == false);
  r.ReadRequest(1);
  r.NextClockCycle();
  assert(r.read_ready() == true);
  assert(r.read_data() == (int) 0xFEEDBEEF);
  r.NextClockCycle();
  assert(r.read_ready() == true);
  assert(r.read_data() == (int) 0xFEEDBEEF);
  r.NextClockCycle();
  assert(r.read_ready() == false);

  // Multiple Writes to Different Addresses
  r.WriteRequest(0, (int) 0xDEADCAFE);
  r.NextClockCycle();
  assert(r.read_ready() == false);
  r.WriteRequest((int) pow(2, ADDRESS_WIDTH) - 1, (int) 0xDEADFEED);
  r.NextClockCycle();
  assert(r.read_ready() == false);
  r.ReadRequest(0);
  r.NextClockCycle();
  assert(r.read_ready() == false);
  r.ReadRequest((int) pow(2, ADDRESS_WIDTH) - 1);
  r.NextClockCycle();
  assert(r.read_ready() == true);
  assert(r.read_data() == (int) 0xDEADCAFE);
  r.NextClockCycle();
  assert(r.read_ready() == true);
  assert(r.read_data() == (int) 0xDEADFEED);
  r.NextClockCycle();
  assert(r.read_ready() == false);
  
  std::cout << "RAM test complete!" << std::endl;
}