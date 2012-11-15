#include <iostream>
#include "fifo.h"
#include <assert.h>

#define FIFO_LENGTH 10
#define FIFO_ALMOST_FULL_LENGTH 8

int main (int argc, char** argv) {
  Fifo<int> f(FIFO_LENGTH, FIFO_ALMOST_FULL_LENGTH);
  
  // Check initial state
  assert(f.IsEmpty() == true && f.IsFull() == false &&
         f.IsAlmostFull() == false);
  
  // Fill and flush test
  for (int i = 0; i < FIFO_LENGTH; i++) {
    f.WriteRequest(i);
    f.NextClockCycle();
    assert(f.IsEmpty() == false);
    if (i != FIFO_LENGTH - 1) {
      assert(f.IsFull() == false);
    }
    if (i >= FIFO_ALMOST_FULL_LENGTH - 1) {
      assert(f.IsAlmostFull() == true);
    } else {
      assert(f.IsAlmostFull() == false);
    }
  }
  assert(f.IsFull() == true);
  for (int i = 0; i < FIFO_LENGTH; i++) {
    assert(f.read_data() == i);
    f.ReadRequest();
    f.NextClockCycle();
    assert(f.IsFull() == false);
    if (i != FIFO_LENGTH - 1) {
      assert(f.IsEmpty() == false);
    }
    if (i >= FIFO_LENGTH - FIFO_ALMOST_FULL_LENGTH) {
      assert(f.IsAlmostFull() == false);
    } else {
      assert(f.IsAlmostFull() == true);
    }
  }
  assert(f.IsEmpty() == true);
  
  std::cout << "FIFO tests complete!" << std::endl;
  return 0;
}