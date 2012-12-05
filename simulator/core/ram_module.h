// File Name         : ram_module.h
// Description       : Multi-port, multi-banked RAM hardware simulation class definitions
// Table of Contents : RamModuleRequest struct declaration
//                     RamModule class declaration
//                     RamModule::RamModule()         - Constructor
//                     RamModule::~RamModule()        - Destructor
//                     RamModule::NextClockCycle()    - Advance to the next clock cycle
//                     RamModule::Reset()             - Reset RAM Module state
//                     RamModule::IsPortReady()       - Check if a port is ready for requests
//                     RamModule::ReadRequest()       - Request to read from RAM
//                     RamModule::ReadReady()         - Check if read data is ready at a port
//                     RamModule::WriteRequest()      - Request to write to RAM
//                     RamModule::ReadData()          - Grab read data at a port
//                     RamModule::Size()              - Get the total size of the RAM module
//                     RamModule::rams()              - Directly access RAMs, used for testing
//                     RamModule::num_rams()          - Accessor for number of RAMs
//                     RamModule::ram_address_width() - Accessor for address width of each RAM
//                     RamModule::Preload()           - Preload the RAM with data
//                     RamModule::GetAccessCount()    - Get the number of accesses for a RAM
//                     RamModule::ResetAccessCounts() - Reset access count for all RAMs
// Revision History  :
//     Albert Ng      Nov 15 2012     Initial Revision
//     Albert Ng      Nov 16 2012     Added RAM module write capability
//     Albert Ng      Nov 21 2012     Added Size()
//     Albert Ng      Nov 22 2012     Added Preload()
//     Albert Ng      Nov 23 2012     Added access count statistics, num_rams(),
//                                      ram_address_width()
//     Albert Ng      Dec 04 2012     Update RAM constructors to use new RAM class with timing
//                                      parameters
//                                    Changed Preload() to use DirectWrite()

#ifndef CS316_CORE_RAM_MODULE_H_
#define CS316_CORE_RAM_MODULE_H_

#include <stdint.h>
#include <cmath>
#include "sequential.h"
#include "ram.h"
#include "fifo.h"
#include <assert.h>
#include "params.h"

template <typename T>
struct RamModuleRequest {
  uint64_t address;
  uint64_t port_num;
  bool is_write;
  T write_data;
};

template <typename T>
struct PortROBEntry {
  uint64_t address;
  T read_data;
  bool read_ready;
};

// Simulates a general multi-port, multi-banked RAM with set number of RAMs, number
// of ports, RAM size, and RAM latency
template <typename T>
class RamModule : public Sequential {
 public:
  RamModule(uint64_t num_rams, uint64_t num_ports, uint64_t addr_row_width,
            uint64_t addr_col_width, uint64_t addr_bank_width,
            uint64_t system_clock_freq_mhz, uint64_t memory_clock_freq_mhz,
            uint64_t tRCD_cycles, uint64_t tCL_cycles, uint64_t tRP_cycles);
  ~RamModule();
  
  // Performs round-robin scheduling of RAM requests and other sequential logic
  void NextClockCycle();

  // Resets all FIFOs and internal states.
  void Reset();
  
  // Checks if port input FIFO is full
  bool IsPortReady(uint64_t port_num);
  
  // Request to read from RAM at a given port.
  void ReadRequest(uint64_t address, uint64_t port_num);
  
  // Checks if the read data is ready.
  bool ReadReady(uint64_t port_num);
  
  // Request to write to RAM at a given port.
  // NOTE: May not work due to variable latencies
  void WriteRequest(uint64_t address, uint64_t port_num, T data);
  
  // Returns the valid data read from the RAM. Asserts to make sure
  // the read data is ready.
  T ReadData(uint64_t port_num);
  
  // Returns the size of the RAM
  uint64_t Size();
  
  // Provides direct access to RAMs. Only used for preloading purposes
  // TODO(Albert): Make a preload function?
  Ram<T>** rams();
  
  // Parameter accessors
  uint64_t num_rams();
  
  uint64_t NumBanks();
  
  // Preload the RAMs with given data
  void Preload(T* data, unsigned int size);
  
  // Get the number of times a RAM has been accessed.
  uint64_t GetAccessCount(uint64_t ram_id);
  
  // Reset access counts for all RAMs
  void ResetAccessCounts();
  
 private:
  // Obtains the RAM chip ID from the passed address.
  uint64_t GetRamID(uint64_t address);
  
  // Obtains the RAM chip address from the passed address.
  uint64_t GetRamAddress(uint64_t address);
  
  // RAM Module setup parameters
  uint64_t num_rams_;
  uint64_t num_ports_;
  uint64_t addr_row_width_;
  uint64_t addr_col_width_;
  uint64_t addr_bank_width_;
  
  // RAM instantiations
  Ram<T>** rams_;
  
  // FIFOs that hold request info for in-flight read requests in each RAM. Used to
  // pass the read request data to the correct port when data is ready.
  Fifo<RamModuleRequest<T> >** ram_inflight_read_request_fifos_;
  
  // FIFOs that contain non-dispatched read requests at each port
  Fifo<RamModuleRequest<T> >** port_input_fifos_;
  
  // Reorder buffers for each port. These are used to report back read data in order
  // because reads have variable latency, and can come back out of order.
  // TODO: Make ROB a class to ensure single push/pop per cycle
  std::list<PortROBEntry<T> >** port_ROBs_;
  static const unsigned int ROB_SIZE = RAM_MODULE_PORT_ROB_SIZE;
  
  // Read data ready flags for each port
  bool* read_ready_;
  
  // Read data for each port
  T* read_data_;
  
  // Port round-robin counters for scheduling read requests into the RAMs.
  uint64_t* port_counters_;
  
  // RAM access counts for statistics
  uint64_t* access_counts_;
};

template <typename T>
RamModule<T>::RamModule(uint64_t num_rams, uint64_t num_ports, uint64_t addr_row_width,
                        uint64_t addr_col_width, uint64_t addr_bank_width,
                        uint64_t system_clock_freq_mhz, uint64_t memory_clock_freq_mhz,
                        uint64_t tRCD_cycles, uint64_t tCL_cycles, uint64_t tRP_cycles) {
  num_rams_ = num_rams;
  num_ports_ = num_ports;
  addr_row_width_ = addr_row_width;
  addr_col_width_ = addr_col_width;
  addr_bank_width_ = addr_bank_width;

  // Initialize RAMs and their associated information
  rams_ = new Ram<T>*[num_rams];
  ram_inflight_read_request_fifos_ = new Fifo<RamModuleRequest<T> >*[num_rams];
  port_counters_ = new uint64_t[num_rams];
  for (unsigned int i = 0; i < num_rams; i++) {
    rams_[i] = new Ram<T>(addr_row_width, addr_col_width, addr_bank_width, system_clock_freq_mhz, memory_clock_freq_mhz, tRCD_cycles, tCL_cycles, tRP_cycles);
    ram_inflight_read_request_fifos_[i] = new Fifo<RamModuleRequest<T> >(RAM_MODULE_INFLIGHT_FIFO_LENGTH);
    port_counters_[i] = 0;
  }
  
  // Initialize ports and their associated information
  port_input_fifos_ = new Fifo<RamModuleRequest<T> >*[num_ports];
  port_ROBs_ = new std::list<PortROBEntry<T> >*[num_ports];
  read_ready_ = new bool[num_ports];
  read_data_ = new T[num_ports];
  for (unsigned int i = 0; i < num_ports_; i++) {
    port_input_fifos_[i] = new Fifo<RamModuleRequest<T> >(RAM_MODULE_PORT_INPUT_FIFO_LENGTH);
    port_ROBs_[i] = new std::list<PortROBEntry<T> >(RAM_MODULE_PORT_ROB_SIZE);
    read_ready_[i] = false;
  }
  
  access_counts_ = new uint64_t[num_rams_];
  ResetAccessCounts();
}

template <typename T>
RamModule<T>::~RamModule() {
  for (unsigned int i = 0; i < num_rams_; i++) {
    delete rams_[i];
    delete ram_inflight_read_request_fifos_[i];
  }
  for (unsigned int i = 0; i < num_ports_; i++) {
    delete port_input_fifos_[i];
    delete port_ROBs_[i];
  }
  delete[] rams_;
  delete[] ram_inflight_read_request_fifos_;
  delete[] port_counters_;
  delete[] port_input_fifos_;
  delete[] port_ROBs_;
  delete[] read_ready_;
  delete[] read_data_;
}

template <typename T>
void RamModule<T>::NextClockCycle() {
  Sequential::NextClockCycle();
  
  // Schedule reads to each RAM
  for (unsigned int i = 0; i < num_rams_; i++) {
    // Loop through ports starting from current port counter and find the
    // first port with an outstanding request to this RAM. If one is found,
    // send that request to the RAM, store it in the inflight request FIFO,
    // pop it from the port input FIFO, and update the current port counter to
    // the next port for fairness.
    // If no ports with an outstanding request for this RAM is found, current
    // port counter stays the same.
    for (unsigned int j = 0; j < num_ports_; j++) {
      int cur_port = (j + port_counters_[i]) % num_ports_;
      if (!(port_input_fifos_[cur_port]->IsEmpty())) {
        RamModuleRequest<T> req = port_input_fifos_[cur_port]->read_data();
        uint64_t ram_id = GetRamID(req.address);
        uint64_t ram_address = GetRamAddress(req.address);
        if (ram_id == i) {
          if ((req.is_write == false) && (port_ROBs_[cur_port]->size() < ROB_SIZE)) {
            // Dispatch read request
            rams_[i]->ReadRequest(ram_address);
            ram_inflight_read_request_fifos_[i]->WriteRequest(req);
            
            // Allocate ROB entry
            PortROBEntry<T> probe;
            probe.address = req.address;
            probe.read_ready = false;
            port_ROBs_[cur_port]->push_back(probe);
            
            port_input_fifos_[cur_port]->ReadRequest();
            port_counters_[i] = (cur_port + 1) % num_ports_;
            access_counts_[i]++;
          } else if (req.is_write == true) {
            rams_[i]->WriteRequest(ram_address, req.write_data);
            
            port_input_fifos_[cur_port]->ReadRequest();
            port_counters_[i] = (cur_port + 1) % num_ports_;
            access_counts_[i]++;
          }

          break;
        }
      }
    }
    
    // When RAM read data is ready, pop the corresponding request information and
    // and send the data to the corresponding port.
    if (rams_[i]->read_ready() == true) {
      RamModuleRequest<T>  req = ram_inflight_read_request_fifos_[i]->read_data();
      ram_inflight_read_request_fifos_[i]->ReadRequest();
      
      // Search port ROB for the corresponding read request and fill the read data
      // and ready flag.
      bool entry_found = false;
      for (typename std::list<PortROBEntry<T> >::iterator it = port_ROBs_[req.port_num]->begin(); it != port_ROBs_[req.port_num]->end(); it++) {
        if (((*it).address == req.address) && ((*it).read_ready == false)) {
          (*it).read_data = rams_[i]->read_data();
          (*it).read_ready = true;
          entry_found = true;
          break;
        }
      }
      assert(entry_found == true);
    }
  }
  
  // Pop from ROBs when oldest read is ready
  for (unsigned int i = 0; i < num_ports_; i++) {
    read_ready_[i] = false;
    
    if (!(port_ROBs_[i]->empty())) {
      PortROBEntry<T> probe = port_ROBs_[i]->front();
      if (probe.read_ready == true) {
        read_ready_[i] = true;
        read_data_[i] = probe.read_data;
        port_ROBs_[i]->pop_front();
      }
    }
  }
  
  for (unsigned int i = 0; i < num_rams_; i++) {
    rams_[i]->NextClockCycle();
    ram_inflight_read_request_fifos_[i]->NextClockCycle();
  }
  for (unsigned int i = 0; i < num_ports_; i++) {
    port_input_fifos_[i]->NextClockCycle();
  }
}

template <typename T>
void RamModule<T>::Reset() {
  for (unsigned int i = 0; i < num_rams_; i++) {
    rams_[i]->Reset();
    ram_inflight_read_request_fifos_[i]->Reset();
    port_counters_[i] = 0;
  }
  for (unsigned int i = 0; i < num_ports_; i++) {
    port_input_fifos_[i]->Reset();
    read_ready_[i] = false;
    port_ROBs_[i]->clear();
  }
  ResetAccessCounts();
}

template <typename T>
bool RamModule<T>::IsPortReady(uint64_t port_num) {
  return !(port_input_fifos_[port_num]->IsFull());
}

template <typename T>
void RamModule<T>::ReadRequest(uint64_t address, uint64_t port_num) {
  RamModuleRequest<T> req;
  req.address = address;
  req.port_num = port_num;
  req.is_write = false;
  port_input_fifos_[port_num]->WriteRequest(req);
  
}

template <typename T>
bool RamModule<T>::ReadReady(uint64_t port_num) {
  return read_ready_[port_num];
}

template <typename T>
void RamModule<T>::WriteRequest(uint64_t address, uint64_t port_num, T data) {
  RamModuleRequest<T> req;
  req.address = address;
  req.port_num = port_num;
  req.is_write = true;
  req.write_data = data;
  port_input_fifos_[port_num]->WriteRequest(req);
}

template <typename T>
T RamModule<T>::ReadData(uint64_t port_num) {
  assert(read_ready_[port_num] == true);
  return read_data_[port_num];
}

template <typename T>
uint64_t RamModule<T>::Size() {
  return (num_rams_ * ((uint64_t) pow(2, addr_row_width_ + addr_col_width_ + addr_bank_width_)));
}

template <typename T>
uint64_t RamModule<T>::GetRamID(uint64_t address) {
  return (address >> (addr_row_width_ + addr_col_width_ + addr_bank_width_));
}

template <typename T>
uint64_t RamModule<T>::GetRamAddress(uint64_t address) {
  return address - (GetRamID(address) << (addr_row_width_ + addr_col_width_ + addr_bank_width_));
}

template <typename T>
Ram<T>** RamModule<T>::rams() {
  return rams_;
}

template <typename T>
uint64_t RamModule<T>::num_rams() {
  return num_rams_;
}

template <typename T>
uint64_t RamModule<T>::NumBanks() {
  return (uint64_t) (pow(2, addr_bank_width_) * num_rams_);
}

template <typename T>
void RamModule<T>::Preload(T* data, unsigned int size) {
  assert(size <= num_rams_ * pow(2, addr_row_width_ + addr_col_width_ + addr_bank_width_));
  
  unsigned int ram_id = 0;
  unsigned int ram_address = 0;
  
  for (unsigned int i = 0; i < size; i++) {
    rams_[ram_id]->DirectWrite(ram_address, data[i]);
    if (ram_address == (uint64_t) (pow(2, addr_row_width_ + addr_col_width_ + addr_bank_width_) - 1)) {
      ram_id++;
      ram_address = 0;
    } else {
      ram_address++;
    }
  }
}

template <typename T>
uint64_t RamModule<T>::GetAccessCount(uint64_t ram_id) {
  return access_counts_[ram_id];
}


template <typename T>
void RamModule<T>::ResetAccessCounts() {
  for (unsigned int i = 0; i < num_rams_; i++) {
    access_counts_[i] = 0;
  }
}
          
#endif // CS316_CORE_RAM_MODULE_H_