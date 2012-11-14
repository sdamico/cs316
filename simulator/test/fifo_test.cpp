#include <iostream>
#include "fifo.h"
#include <assert.h>

#define FIFO_LENGTH 10

int main (int argc, char** argv) {
  Fifo<int> f(FIFO_LENGTH);
  int read_test;
  
  // Check initial state
  assert(f.IsEmpty() == true && f.IsFull() == false);
  
  // Fill and flush test
  for (int i = 0; i < FIFO_LENGTH; i++) {
    f.WriteRequest(i);
    f.NextClockCycle();
    assert(f.IsEmpty() == false);
    if (i != FIFO_LENGTH - 1) {
      assert(f.IsFull() == false);
    }
  }
  assert(f.IsFull() == true);
  for (int i = 0; i < FIFO_LENGTH; i++) {
    f.ReadRequest(&read_test);
    f.NextClockCycle();
    assert(read_test == i);
    assert(f.IsFull() == false);
    if (i != FIFO_LENGTH - 1) {
      assert(f.IsEmpty() == false);
    }
  }
  assert(f.IsEmpty() == true);
  
  // Write when full test
  for (int i = 0; i < FIFO_LENGTH + 1; i++) {
    f.WriteRequest(i);
    f.NextClockCycle();
  }
  assert(f.IsFull() == true);
  for (int i = 0; i < FIFO_LENGTH; i++) {
    f.ReadRequest(&read_test);
    f.NextClockCycle();
    assert(read_test == i);
  }
  assert(f.IsEmpty() == true);
  
  // Read when empty test
  read_test = -1;
  f.ReadRequest(&read_test);
  f.NextClockCycle();
  assert(f.IsEmpty() == true);
  assert(read_test == -1);
  
  
  std::cout << "FIFO tests complete!" << std::endl;
  return 0;
}