// TODO(Albert): Complete implementation

#ifndef CS316_CORE_RAM_MODULE_H_
#define CS316_CORE_RAM_MODULE_H_

#include <stdint.h>
#include "sequential.h"
#include "ram.h"
#include "fifo.h"

struct RamModuleRequest {
  uint64_t address;
  uint64_t port_num;
};

template <typename T>
class RamModule : public Sequential {
 public:
  RamModule(uint64_t num_rams, uint64_t num_ports, uint64_t ram_address_width,
             uint8_t ram_latency);
  ~RamModule();
  
  // Bitch.
  void NextClockCycle();

  // Resets all FIFOs and internal states.
  void Reset();
  
  // Checks if port input FIFO is full
  bool IsPortReady(uint64_t port_num);
  
  // Request to read from RAM at a given port.
  void ReadRequest(uint64_t address, uint64_t port_num);
  
  // Checks if the read data is ready.
  bool ReadReady(uint64_t port_num);
  
  // Returns the valid data read from the RAM. Asserts to make sure
  // the read data is ready.
  T ReadData(uint64_t port_num);
  
  // ONLY USED FOR TESTING PURPOSES
  Ram<T>** GetRams();
  
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
  
  // FIFOs that hold request info for in-flight requests in each RAM. Used to
  // pass the request data to the correct port when data is ready. FIFOs
  // are each length ram latency.
  Fifo<RamModuleRequest>** ram_inflight_request_fifos_;
  
  // Port Input FIFOs are each length num_ports. This FIFO handles
  // contentions at the port end between multiple ports requesting reads
  // to the same RAM block, because only one request can be pushed into
  // a RAM input FIFO per clock. With a length of num_ports, the FIFO can
  // handle all of the ports requesting a read to the same RAM in one burst,
  // but not continuously.
  Fifo<RamModuleRequest>** port_input_fifos_;
  
  // Read data ready flags for each port
  bool* read_ready_;
  
  // Read data for each port
  T* read_data_;
  
  // Port round-robin counters for scheduling read requests into the RAMs
  uint64_t* port_counters_;
};

template <typename T>
RamModule<T>::RamModule(uint64_t num_rams, uint64_t num_ports,
                        uint64_t ram_address_width, uint8_t ram_latency) {
  num_rams_ = num_rams;
  num_ports_ = num_ports;
  ram_address_width_ = ram_address_width;
  ram_latency_ = ram_latency;

  rams_ = new Ram<T>*[num_rams];
  ram_inflight_request_fifos_ = new Fifo<RamModuleRequest>*[num_rams];
  port_counters_ = new uint64_t[num_rams];
  for (unsigned int i = 0; i < num_rams; i++) {
    rams_[i] = new Ram<T>(ram_address_width_, ram_latency_);
    ram_inflight_request_fifos_[i] = new Fifo<RamModuleRequest>(ram_latency_ + 1);
    port_counters_[i] = 0;
  }
  port_input_fifos_ = new Fifo<RamModuleRequest>*[num_ports];
  read_ready_ = new bool[num_ports];
  read_data_ = new T[num_ports];
  for (unsigned int i = 0; i < num_ports_; i++) {
    port_input_fifos_[i] = new Fifo<RamModuleRequest>(num_ports);
    assert(!(port_input_fifos_[i]->IsFull()));
    read_ready_[i] = false;
  }
}

template <typename T>
RamModule<T>::~RamModule() {
  for (unsigned int i = 0; i < num_rams_; i++) {
    delete rams_[i];
    delete ram_inflight_request_fifos_[i];
  }
  for (unsigned int i = 0; i < num_ports_; i++) {
    delete port_input_fifos_[i];
  }
  delete rams_;
  delete ram_inflight_request_fifos_;
  delete port_input_fifos_;
  delete read_ready_;
  delete read_data_;
}

template <typename T>
void RamModule<T>::Reset() {
  for (unsigned int i = 0; i < num_rams_; i++) {
    rams_[i]->Reset();
    ram_inflight_request_fifos_[i]->Reset();
    port_counters_[i] = 0;
  }
  for (unsigned int i = 0; i < num_ports_; i++) {
    port_input_fifos_[i]->Reset();
    read_ready_[i] = false;
  }
}

template <typename T>
bool RamModule<T>::IsPortReady(uint64_t port_num) {
  return !(port_input_fifos_[port_num]->IsFull());
}

template <typename T>
void RamModule<T>::ReadRequest(uint64_t address, uint64_t port_num) {
  RamModuleRequest req;
  req.address = address;
  req.port_num = port_num;
  port_input_fifos_[port_num]->WriteRequest(req);
}

template <typename T>
bool RamModule<T>::ReadReady(uint64_t port_num) {
  return read_ready_[port_num];
}

template <typename T>
T RamModule<T>::ReadData(uint64_t port_num) {
  assert(read_ready_[port_num] == true);
  return read_data_[port_num];
}

template <typename T>
void RamModule<T>::NextClockCycle() {
  Sequential::NextClockCycle();
  for (unsigned int i = 0; i < num_ports_; i++) {
    read_ready_[i] = false;
  }
  
  for (unsigned int i = 0; i < num_rams_; i++) {
    // Loop through ports starting from current port counter and find the
    // first port with an outstanding read request to this RAM. If one is found,
    // send that read request to the RAM, store it in the inflight request FIFO,
    // pop it from the port input FIFO, and update the current port counter to
    // the next port for fairness.
    // If no ports with an outstanding request for this RAM is found, current
    // port counter stays the same.
    for (unsigned int j = 0; j < num_ports_; j++) {
      int cur_port = (j + port_counters_[i]) % num_ports_;
      RamModuleRequest req = port_input_fifos_[cur_port]->read_data();
      uint64_t ram_id = GetRamID(req.address);
      uint64_t ram_address = GetRamAddress(req.address);
      if (!(port_input_fifos_[cur_port]->IsEmpty()) && ram_id == i) {
        rams_[i]->ReadRequest(ram_address);
        ram_inflight_request_fifos_[i]->WriteRequest(req);
        port_input_fifos_[cur_port]->ReadRequest();
        port_counters_[i] = (cur_port + 1) % num_ports_;
        break;
      }
    }
    
    // When RAM data is ready, pop the corresponding request information and
    // and send the data to the corresponding port.
    if (rams_[i]->read_ready() == true) {
      RamModuleRequest req = ram_inflight_request_fifos_[i]->read_data();
      ram_inflight_request_fifos_[i]->ReadRequest();
      read_ready_[req.port_num] = true;
      read_data_[req.port_num] = rams_[i]->read_data();
    }
  } 
  
  for (unsigned int i = 0; i < num_rams_; i++) {
    rams_[i]->NextClockCycle();
    ram_inflight_request_fifos_[i]->NextClockCycle();
  }
  for (unsigned int i = 0; i < num_ports_; i++) {
    port_input_fifos_[i]->NextClockCycle();
  }
}

template <typename T>
uint64_t RamModule<T>::GetRamID(uint64_t address) {
  //std::cout << "Getting Ram ID from address " << address << ": " << (address >> ram_address_width_) << std::endl;
  return (address >> ram_address_width_);
            
}

template <typename T>
uint64_t RamModule<T>::GetRamAddress(uint64_t address) {
  return address - (GetRamID(address) << ram_address_width_);
}

template <typename T>
Ram<T>** RamModule<T>::GetRams() {
  return rams_;
}
          
#endif // CS316_CORE_RAM_MODULE_H_