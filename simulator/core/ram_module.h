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

#ifndef CS316_CORE_RAM_MODULE_H_
#define CS316_CORE_RAM_MODULE_H_

#include <stdint.h>
#include <cmath>
#include "sequential.h"
#include "ram.h"
#include "fifo.h"
#include <assert.h>

template <typename T>
struct RamModuleRequest {
  uint64_t address;
  uint64_t port_num;
  bool is_write;
  T write_data;
};

// Simulates a general multi-port, multi-banked RAM with set number of RAMs, number
// of ports, RAM size, and RAM latency
template <typename T>
class RamModule : public Sequential {
 public:
  RamModule(uint64_t num_rams, uint64_t num_ports, uint64_t ram_address_width,
            uint8_t ram_latency);
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
  uint64_t ram_address_width();
  
  // Preload the RAMs with given data, with or without interleaving.
  void Preload(T* data, unsigned int size, bool interleave);
  
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
  uint64_t ram_address_width_;
  uint8_t ram_latency_;
  
  // RAM instantiations
  Ram<T>** rams_;
  
  // FIFOs that hold request info for in-flight read requests in each RAM. Used to
  // pass the read request data to the correct port when data is ready. FIFOs
  // are each length ram latency.
  Fifo<RamModuleRequest<T> >** ram_inflight_read_request_fifos_;
  
  // Port Input FIFOs are each length num_ports. This FIFO handles
  // contentions at the port end between multiple ports requesting access
  // to the same RAM block, because only one request can be pushed into
  // a RAM input FIFO per clock. With a length of num_ports, the FIFO can
  // handle all of the ports requesting access to the same RAM in one burst,
  // but not continuously.
  Fifo<RamModuleRequest<T> >** port_input_fifos_;
  
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
RamModule<T>::RamModule(uint64_t num_rams, uint64_t num_ports,
                        uint64_t ram_address_width, uint8_t ram_latency) {
  num_rams_ = num_rams;
  num_ports_ = num_ports;
  ram_address_width_ = ram_address_width;
  ram_latency_ = ram_latency;

  rams_ = new Ram<T>*[num_rams];
  ram_inflight_read_request_fifos_ = new Fifo<RamModuleRequest<T> >*[num_rams];
  port_counters_ = new uint64_t[num_rams];
  for (unsigned int i = 0; i < num_rams; i++) {
    rams_[i] = new Ram<T>(ram_address_width_, ram_latency_);
    ram_inflight_read_request_fifos_[i] = new Fifo<RamModuleRequest<T> >(ram_latency_ + 1);
    port_counters_[i] = 0;
  }
  port_input_fifos_ = new Fifo<RamModuleRequest<T> >*[num_ports];
  read_ready_ = new bool[num_ports];
  read_data_ = new T[num_ports];
  for (unsigned int i = 0; i < num_ports_; i++) {
    port_input_fifos_[i] = new Fifo<RamModuleRequest<T> >(num_ports);
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
  }
  delete rams_;
  delete ram_inflight_read_request_fifos_;
  delete port_counters_;
  delete port_input_fifos_;
  delete read_ready_;
  delete read_data_;
}

template <typename T>
void RamModule<T>::NextClockCycle() {
  Sequential::NextClockCycle();
  for (unsigned int i = 0; i < num_ports_; i++) {
    read_ready_[i] = false;
  }
  
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
      RamModuleRequest<T> req = port_input_fifos_[cur_port]->read_data();
      uint64_t ram_id = GetRamID(req.address);
      uint64_t ram_address = GetRamAddress(req.address);
      if (!(port_input_fifos_[cur_port]->IsEmpty()) && ram_id == i) {
        if (req.is_write == false) {
          rams_[i]->ReadRequest(ram_address);
          ram_inflight_read_request_fifos_[i]->WriteRequest(req);
        } else {
          rams_[i]->WriteRequest(ram_address, req.write_data);
        }
        
        port_input_fifos_[cur_port]->ReadRequest();
        port_counters_[i] = (cur_port + 1) % num_ports_;
        access_counts_[i]++;
        break;
      }
    }
    
    // When RAM read data is ready, pop the corresponding request information and
    // and send the data to the corresponding port.
    if (rams_[i]->read_ready() == true) {
      RamModuleRequest<T>  req = ram_inflight_read_request_fifos_[i]->read_data();
      ram_inflight_read_request_fifos_[i]->ReadRequest();
      read_ready_[req.port_num] = true;
      read_data_[req.port_num] = rams_[i]->read_data();
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
  return (num_rams_ * ((uint64_t) pow(2, ram_address_width_)));
}

template <typename T>
uint64_t RamModule<T>::GetRamID(uint64_t address) {
  return (address >> ram_address_width_);
}

template <typename T>
uint64_t RamModule<T>::GetRamAddress(uint64_t address) {
  return address - (GetRamID(address) << ram_address_width_);
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
uint64_t RamModule<T>::ram_address_width() {
  return ram_address_width_;
}

template <typename T>
void RamModule<T>::Preload(T* data, unsigned int size, bool interleave) {
  assert(size <= num_rams_ * pow(2, ram_address_width_));
  
  unsigned int ram_id = 0;
  unsigned int* ram_addresses = new unsigned int[num_rams_];
  for (unsigned int i = 0; i < num_rams_; i++) {
    ram_addresses[i] = 0;
  }
  
  for (unsigned int i = 0; i < size; i++) {
    rams_[ram_id]->WriteRequest(ram_addresses[ram_id], data[i]);
    rams_[ram_id]->NextClockCycle();
    if (interleave == false) {
      if (ram_addresses[ram_id] == (uint64_t) (pow(2, ram_address_width_) - 1)) {
        ram_id++;
      } else {
        ram_addresses[ram_id]++;
      }
    } else {
      ram_addresses[ram_id]++;
      ram_id = (ram_id + 1) % num_rams_;
    }
  }
  for (unsigned int i = 0; i < ram_latency_; i++) {
    for (unsigned int j = 0; j < num_rams_; j++) {
      rams_[j]->NextClockCycle();
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