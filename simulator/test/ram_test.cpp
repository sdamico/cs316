#include <iostream>
#include "ram.h"
#include <assert.h>
#include <cmath>

#define ADDR_ROW_WIDTH 2
#define ADDR_COL_WIDTH 2
#define ADDR_BANK_WIDTH 2
#define ADDRESS_WIDTH (ADDR_ROW_WIDTH+ADDR_COL_WIDTH+ADDR_BANK_WIDTH)

#define TRCD 16
#define TCL 14
#define TRP 16
#define SYSTEM_CLK_FREQ 400
#define MEM_CLK_FREQ 400

int main (int arg, char** argv) {
//  Ram(uint64_t addr_row_width, uint64_t addr_col_width, uint64_t addr_bank_width, 
//    uint64_t system_clock_freq_mhz, uint64_t memory_clock_freq_mhz,
//    uint64_t tRCD_cycles, uint64_t tCL_cycles, uint64_t tRP_cycles);
  Ram<int> r(ADDR_ROW_WIDTH, ADDR_COL_WIDTH, ADDR_BANK_WIDTH,
              SYSTEM_CLK_FREQ, MEM_CLK_FREQ,
              TRCD, TCL, TRP);
  
  // Check initial states
  assert(r.read_ready() == false);
  
  // Read after Write Test
  for (int i = 0; i < pow(2, ADDRESS_WIDTH); i++) {
    r.WriteRequest(i, i);
    r.NextClockCycle();
  }    
  for (int i = 0; i < (TRCD+TCL+TRP)*pow(2, ADDRESS_WIDTH)*10; i++) {
    r.NextClockCycle();
  }
  int start_cycle = r.cycle_count();
  assert(r.read_ready() == false);
  for (int col = 0; col < pow(2, ADDR_COL_WIDTH); col++) {
    for (int row = 0; row < pow(2, ADDR_ROW_WIDTH); row++) {
      for (int bank = 0; bank < pow(2, ADDR_BANK_WIDTH); bank++) {
        uint64_t address = (bank << (ADDR_ROW_WIDTH+ADDR_COL_WIDTH)) | (row << (ADDR_COL_WIDTH)) | col;
        r.ReadRequest(address);
        do {
          r.NextClockCycle();          
        } while (r.read_ready() == false);
        assert(r.read_data() == address);
      }
    }  
  }
  
  //same row, same bank
  r.Reset();
  start_cycle = r.cycle_count();
  r.ReadRequest(0);
  r.NextClockCycle();    
  r.ReadRequest(1);  
  r.NextClockCycle();    
  r.ReadRequest(2);  
  do {
    r.NextClockCycle();          
  } while (r.read_ready() == false);

  assert (r.cycle_count() - start_cycle == TRCD + TCL + TRP+1); //off by 1 to start pipeline to clock into fifo
  start_cycle = r.cycle_count();
  do {
    r.NextClockCycle();          
  } while (r.read_ready() == false);

  assert (r.cycle_count() - start_cycle == Ram<int>::BURST_LENGTH/2);
  start_cycle = r.cycle_count();
  do {
    r.NextClockCycle();          
  } while (r.read_ready() == false);

  assert (r.cycle_count() - start_cycle == Ram<int>::BURST_LENGTH/2);
  
  //different row, same bank
  r.Reset();
  start_cycle = r.cycle_count();
  r.ReadRequest(0);
  r.NextClockCycle();    
  r.ReadRequest((int)pow(2,ADDR_COL_WIDTH));  
  r.NextClockCycle();    
  r.ReadRequest((int)pow(2,ADDR_COL_WIDTH)*2);  
  do {
    r.NextClockCycle();          
  } while (r.read_ready() == false);

  assert (r.cycle_count() - start_cycle == TRCD + TCL + TRP+1); //off by 1 to start pipeline to clock into fifo
  start_cycle = r.cycle_count();
  do {
    r.NextClockCycle();          
  } while (r.read_ready() == false);

  assert (r.cycle_count() - start_cycle == TRCD + TCL + TRP);
  start_cycle = r.cycle_count();
  do {
    r.NextClockCycle();          
  } while (r.read_ready() == false);

  assert (r.cycle_count() - start_cycle == TRCD + TCL + TRP);
  
  //different bank, same row, separated by different bank read
  r.Reset();
  start_cycle = r.cycle_count();
  r.ReadRequest(0);
  r.NextClockCycle();    
  r.ReadRequest((int)pow(2,ADDR_COL_WIDTH+ADDR_ROW_WIDTH));  
  r.NextClockCycle();    
  r.ReadRequest(1);  
  r.NextClockCycle();    
  r.ReadRequest((int)pow(2,ADDR_COL_WIDTH+ADDR_ROW_WIDTH)+1);  
  do {
    r.NextClockCycle();          
  } while (r.read_ready() == false);

  assert (r.cycle_count() - start_cycle == TRCD + TCL + TRP+1); //off by 1 to start pipeline to clock into fifo
  start_cycle = r.cycle_count();
  do {
    r.NextClockCycle();          
  } while (r.read_ready() == false);

  assert (r.cycle_count() - start_cycle == Ram<int>::BURST_LENGTH/2); //check for burst rate limiting
  start_cycle = r.cycle_count();
  do {
    r.NextClockCycle();          
  } while (r.read_ready() == false);

  assert (r.cycle_count() - start_cycle == Ram<int>::BURST_LENGTH/2); 
  start_cycle = r.cycle_count();
  do {
    r.NextClockCycle();          
  } while (r.read_ready() == false);

  assert (r.cycle_count() - start_cycle == Ram<int>::BURST_LENGTH/2);

  //different bank, same row, separated by different bank read
  r.Reset();
  start_cycle = r.cycle_count();
  r.ReadRequest(0);
  r.NextClockCycle();    
  r.ReadRequest((int)pow(2,ADDR_COL_WIDTH+ADDR_ROW_WIDTH));  
  r.NextClockCycle();    
  r.ReadRequest(pow(2,ADDR_COL_WIDTH));  
  r.NextClockCycle();    
  r.ReadRequest((int)pow(2,ADDR_COL_WIDTH+ADDR_ROW_WIDTH)+pow(2,ADDR_COL_WIDTH));  
  do {
    r.NextClockCycle();          
  } while (r.read_ready() == false);

  assert (r.cycle_count() - start_cycle == TRCD + TCL + TRP+1); //off by 1 to start pipeline to clock into fifo
  start_cycle = r.cycle_count();
  do {
    r.NextClockCycle();          
  } while (r.read_ready() == false);

  assert (r.cycle_count() - start_cycle == Ram<int>::BURST_LENGTH/2); //check for burst rate limiting
  start_cycle = r.cycle_count();
  do {
    r.NextClockCycle();          
  } while (r.read_ready() == false);

  assert (r.cycle_count() - start_cycle == TRCD + TCL + TRP - Ram<int>::BURST_LENGTH/2); 
  start_cycle = r.cycle_count();
  do {
    r.NextClockCycle();          
  } while (r.read_ready() == false);

  assert (r.cycle_count() - start_cycle == Ram<int>::BURST_LENGTH/2);
  
  //bank 0, bank 1, bank 2, bank 0
  r.Reset();
  start_cycle = r.cycle_count();
  r.ReadRequest(0);
  r.NextClockCycle();    
  r.ReadRequest((int)pow(2,ADDR_COL_WIDTH+ADDR_ROW_WIDTH));  
  r.NextClockCycle();    
  r.ReadRequest((int)pow(2,ADDR_COL_WIDTH+ADDR_ROW_WIDTH)*2);  
  r.NextClockCycle();    
  r.ReadRequest((int)pow(2,ADDR_COL_WIDTH));  
  do {
    r.NextClockCycle();          
  } while (r.read_ready() == false);

  assert (r.cycle_count() - start_cycle == TRCD + TCL + TRP+1); //off by 1 to start pipeline to clock into fifo
  start_cycle = r.cycle_count();
  do {
    r.NextClockCycle();          
  } while (r.read_ready() == false);

  assert (r.cycle_count() - start_cycle == Ram<int>::BURST_LENGTH/2); //check for burst rate limiting
  start_cycle = r.cycle_count();
  do {
    r.NextClockCycle();          
  } while (r.read_ready() == false);

  assert (r.cycle_count() - start_cycle == Ram<int>::BURST_LENGTH/2); 
  start_cycle = r.cycle_count();
  do {
    r.NextClockCycle();          
  } while (r.read_ready() == false);

  int expected = Ram<int>::BURST_LENGTH/2;
  int expected1 = TRCD + TCL + TRP - Ram<int>::BURST_LENGTH/2*2;
  if ( expected1 > Ram<int>::BURST_LENGTH/2) {
    expected = expected1;
  }
  assert (r.cycle_count() - start_cycle == expected);


  // halving system clock frequency

  Ram<int> r2(ADDR_ROW_WIDTH, ADDR_COL_WIDTH, ADDR_BANK_WIDTH,
              SYSTEM_CLK_FREQ/2, MEM_CLK_FREQ,
              TRCD, TCL, TRP);
  
  // Check initial states
  assert(r2.read_ready() == false);
  
  // Read after Write Test
  for (int i = 0; i < pow(2, ADDRESS_WIDTH); i++) {
    r2.WriteRequest(i, i);
    r2.NextClockCycle();
  }    
  for (int i = 0; i < (TRCD+TCL+TRP)*pow(2, ADDRESS_WIDTH)*10; i++) {
    r2.NextClockCycle();
  }
  start_cycle = r2.cycle_count();
  assert(r2.read_ready() == false);
  for (int col = 0; col < pow(2, ADDR_COL_WIDTH); col++) {
    for (int row = 0; row < pow(2, ADDR_ROW_WIDTH); row++) {
      for (int bank = 0; bank < pow(2, ADDR_BANK_WIDTH); bank++) {
        uint64_t address = (bank << (ADDR_ROW_WIDTH+ADDR_COL_WIDTH)) | (row << (ADDR_COL_WIDTH)) | col;
        r2.ReadRequest(address);
        do {
          r2.NextClockCycle();          
        } while (r2.read_ready() == false);
        assert(r2.read_data() == address);
      }
    }  
  }
  
  //same row, same bank
  r2.Reset();
  start_cycle = r2.cycle_count();
  r2.ReadRequest(0);
  r2.NextClockCycle();    
  r2.ReadRequest(1);  
  r2.NextClockCycle();    
  r2.ReadRequest(2);  
  do {
    r2.NextClockCycle();          
  } while (r2.read_ready() == false);

  assert (r2.cycle_count() - start_cycle == (TRCD + TCL + TRP)/2+1); //off by 1 to start pipeline to clock into fifo
  start_cycle = r2.cycle_count();
  do {
    r2.NextClockCycle();          
  } while (r2.read_ready() == false);

  assert (r2.cycle_count() - start_cycle == Ram<int>::BURST_LENGTH/4);
  start_cycle = r2.cycle_count();
  do {
    r2.NextClockCycle();          
  } while (r2.read_ready() == false);

  assert (r2.cycle_count() - start_cycle == Ram<int>::BURST_LENGTH/4);
  
  //different row, same bank
  r2.Reset();
  start_cycle = r2.cycle_count();
  r2.ReadRequest(0);
  r2.NextClockCycle();    
  r2.ReadRequest((int)pow(2,ADDR_COL_WIDTH));  
  r2.NextClockCycle();    
  r2.ReadRequest((int)pow(2,ADDR_COL_WIDTH)*2);  
  do {
    r2.NextClockCycle();          
  } while (r2.read_ready() == false);

  assert (r2.cycle_count() - start_cycle == (TRCD + TCL + TRP)/2+1); //off by 1 to start pipeline to clock into fifo
  start_cycle = r2.cycle_count();
  do {
    r2.NextClockCycle();          
  } while (r2.read_ready() == false);

  assert (r2.cycle_count() - start_cycle == (TRCD + TCL + TRP)/2);
  start_cycle = r2.cycle_count();
  do {
    r2.NextClockCycle();          
  } while (r2.read_ready() == false);

  assert (r2.cycle_count() - start_cycle == (TRCD + TCL + TRP)/2);
  
  //different bank, same row, separated by different bank read
  r2.Reset();
  start_cycle = r2.cycle_count();
  r2.ReadRequest(0);
  r2.NextClockCycle();    
  r2.ReadRequest((int)pow(2,ADDR_COL_WIDTH+ADDR_ROW_WIDTH));  
  r2.NextClockCycle();    
  r2.ReadRequest(1);  
  r2.NextClockCycle();    
  r2.ReadRequest((int)pow(2,ADDR_COL_WIDTH+ADDR_ROW_WIDTH)+1);  
  do {
    r2.NextClockCycle();          
  } while (r2.read_ready() == false);

  assert (r2.cycle_count() - start_cycle == (TRCD + TCL + TRP)/2+1); //off by 1 to start pipeline to clock into fifo
  start_cycle = r2.cycle_count();
  do {
    r2.NextClockCycle();          
  } while (r2.read_ready() == false);

  assert (r2.cycle_count() - start_cycle == Ram<int>::BURST_LENGTH/4); //check for burst rate limiting
  start_cycle = r2.cycle_count();
  do {
    r2.NextClockCycle();          
  } while (r2.read_ready() == false);

  assert (r2.cycle_count() - start_cycle == Ram<int>::BURST_LENGTH/4); 
  start_cycle = r2.cycle_count();
  do {
    r2.NextClockCycle();          
  } while (r2.read_ready() == false);

  assert (r2.cycle_count() - start_cycle == Ram<int>::BURST_LENGTH/4);

  //different bank, same row, separated by different bank read
  r2.Reset();
  start_cycle = r2.cycle_count();
  r2.ReadRequest(0);
  r2.NextClockCycle();    
  r2.ReadRequest((int)pow(2,ADDR_COL_WIDTH+ADDR_ROW_WIDTH));  
  r2.NextClockCycle();    
  r2.ReadRequest(pow(2,ADDR_COL_WIDTH));  
  r2.NextClockCycle();    
  r2.ReadRequest((int)pow(2,ADDR_COL_WIDTH+ADDR_ROW_WIDTH)+pow(2,ADDR_COL_WIDTH));  
  do {
    r2.NextClockCycle();          
  } while (r2.read_ready() == false);

  assert (r2.cycle_count() - start_cycle == (TRCD + TCL + TRP)/2+1); //off by 1 to start pipeline to clock into fifo
  start_cycle = r2.cycle_count();
  do {
    r2.NextClockCycle();          
  } while (r2.read_ready() == false);

  assert (r2.cycle_count() - start_cycle == Ram<int>::BURST_LENGTH/4); //check for burst rate limiting
  start_cycle = r2.cycle_count();
  do {
    r2.NextClockCycle();          
  } while (r2.read_ready() == false);

  assert (r2.cycle_count() - start_cycle == (TRCD + TCL + TRP - Ram<int>::BURST_LENGTH/2)/2); 
  start_cycle = r2.cycle_count();
  do {
    r2.NextClockCycle();          
  } while (r2.read_ready() == false);

  assert (r2.cycle_count() - start_cycle == Ram<int>::BURST_LENGTH/4);
  
  //bank 0, bank 1, bank 2, bank 0
  r2.Reset();
  start_cycle = r2.cycle_count();
  r2.ReadRequest(0);
  r2.NextClockCycle();    
  r2.ReadRequest((int)pow(2,ADDR_COL_WIDTH+ADDR_ROW_WIDTH));  
  r2.NextClockCycle();    
  r2.ReadRequest((int)pow(2,ADDR_COL_WIDTH+ADDR_ROW_WIDTH)*2);  
  r2.NextClockCycle();    
  r2.ReadRequest((int)pow(2,ADDR_COL_WIDTH));  
  do {
    r2.NextClockCycle();          
  } while (r2.read_ready() == false);

  assert (r2.cycle_count() - start_cycle == (TRCD + TCL + TRP)/2+1); //off by 1 to start pipeline to clock into fifo
  start_cycle = r2.cycle_count();
  do {
    r2.NextClockCycle();          
  } while (r2.read_ready() == false);

  assert (r2.cycle_count() - start_cycle == Ram<int>::BURST_LENGTH/4); //check for burst rate limiting
  start_cycle = r2.cycle_count();
  do {
    r2.NextClockCycle();          
  } while (r2.read_ready() == false);

  assert (r2.cycle_count() - start_cycle == Ram<int>::BURST_LENGTH/4); 
  start_cycle = r2.cycle_count();
  do {
    r2.NextClockCycle();          
  } while (r2.read_ready() == false);

  expected = Ram<int>::BURST_LENGTH/4;
  expected1 = (TRCD + TCL + TRP)/2 - Ram<int>::BURST_LENGTH/4*2;
  if ( expected1 > Ram<int>::BURST_LENGTH/4) {
    expected = expected1;
  }
  assert (r2.cycle_count() - start_cycle == expected);
  
  std::cout << "RAM test complete!" << std::endl;
}