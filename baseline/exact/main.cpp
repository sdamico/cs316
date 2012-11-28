#include "table_io.h"
#include "def.h"
#include <cmath>
#include <iostream>
#include <fstream>
#include <stdint.h>
#include <vector>
#include <assert.h>
#include <cstdlib>

// Merges two sorted lists of positions, given a required offset (vec2 - vec1)
void merge (std::vector<unsigned int>* vec1, std::vector<unsigned int>* vec2, std::vector<unsigned int>* result, unsigned int offset) {
  unsigned int ptr1 = 0;
  unsigned int ptr2 = 0;
  
  while ((*vec2)[ptr2] < offset) {
    ptr2++;
  }
  while ((ptr1 < vec1->size()) && (ptr2 < vec2->size())) {
    if ((*vec1)[ptr1] == (*vec2)[ptr2] - offset) {
      result->push_back((*vec1)[ptr1]);
      ptr1++;
      ptr2++;
    } else if ((*vec1)[ptr1] > ((*vec2)[ptr2] - offset)) {
      ptr2++;
    } else {
      ptr1++;
    }
  }
}

int main (int argc, char** argv) {
  if (argc < 5) {
    std::cout << "Usage: " << argv[0] << " <Subread Length> <Interval Table Filename> <Position Table Filename> <Queries Filename> <Output Filename> [Subread Filename]" << std::endl;
    exit(1);
  }
  
  std::ifstream queries_file;
  unsigned int num_queries;
  unsigned int query_length;
  queries_file.open(argv[4]);
  queries_file.read((char *)(&num_queries), sizeof(unsigned int));
  queries_file.read((char *)(&query_length), sizeof(unsigned int));
  
  unsigned int subread_length = atoi(argv[1]);
  unsigned int num_subreads_per_query = query_length / subread_length; // Truncating partial subreads
  
  // Read in query list
  std::cout << "Reading query list" << std::endl;
  query_list qlist;
  qlist.num_queries = num_queries;
  qlist.query_length = query_length;
  unsigned int bytes_per_query = (unsigned int) ceil((float)query_length/4);
  qlist.ptr = new unsigned char*[num_queries];
  for (unsigned int i = 0; i < num_queries; i++) {
    qlist.ptr[i] = new unsigned char[bytes_per_query];
    queries_file.read((char *)(qlist.ptr[i]), bytes_per_query * sizeof(unsigned char));
  }
  
  // Split query list into subread list
  std::cout << "Splitting query list into subread list" << std::endl;
  subread_list srlist;
  srlist.num_queries = num_queries;
  srlist.num_subreads_per_query = num_subreads_per_query;
  
  srlist.ptr = new uint32_t*[num_queries];
  for (unsigned int i = 0; i < num_queries; i++) {
    srlist.ptr[i] = new uint32_t[num_subreads_per_query];
  }
  for (unsigned int i = 0; i < num_queries; i++) {
    unsigned int bit_index = 0;
    
    for (unsigned int j = 0 ; j < num_subreads_per_query; j++) {
      unsigned int byte = bit_index / 8;
      unsigned int offset = bit_index % 8;
      uint32_t subread = 0;
      subread += qlist.ptr[i][byte] & (0xFF >> offset);
      unsigned int bits_read = 8 - offset;
      byte++;
      while ((bits_read < subread_length*2) && (subread_length*2 - bits_read >= 8)) {
        subread <<= 8;
        subread += qlist.ptr[i][byte];
        byte++;
        bits_read += 8;
      }
      if (subread_length*2 - bits_read > 0) {
        subread <<= (subread_length*2 - bits_read);
        subread += qlist.ptr[i][byte] >> (8 - (subread_length*2 - bits_read));
        bits_read += (subread_length*2 - bits_read);
      }
      srlist.ptr[i][j] = subread;
      bit_index += (subread_length*2);
    }
  }

  // Write subread list into ascii file
  if (argc == 7) {
    std::ofstream subread_file;
    subread_file.open(argv[6]);
    subread_file << num_queries << std::endl;
    subread_file << query_length << std::endl;
    subread_file << subread_length << std::endl;
    for (unsigned int i = 0 ; i < num_queries; i++) {
      for (unsigned int j = 0; j < num_subreads_per_query; j++) {
        unsigned int subread_shifted = srlist.ptr[i][j] << (sizeof(uint32_t)*8 - subread_length*2);
        for (unsigned int k = 0 ; k < subread_length; k++) {
          unsigned int nucleotide = (subread_shifted & (0xC0000000)) >> 30;
          switch (nucleotide) {
            case 0 : subread_file << 'A'; break;
            case 1 : subread_file << 'C'; break;
            case 2 : subread_file << 'G'; break;
            case 3 : subread_file << 'T'; break;
            default : subread_file << 'X'; break;
          }
          subread_shifted <<= 2;
        }
        subread_file << ' ';
      }
      subread_file << std::endl;
    }
    subread_file.close();
  }

  // Deallocate query list
  for (unsigned int i = 0; i < num_queries; i++) {
    delete[] qlist.ptr[i];
  }
  delete[] qlist.ptr;
  
  // Read in Interval and Position Tables
  std::cout << "Reading interval and position tables" << std::endl;
  table interval_table;
  table position_table;
  ReadIntervalTable(argv[2], &interval_table);
  ReadPositionTable(argv[3], &position_table);
  
  // Look up intervals for each subread
  std::cout << "Performing interval table lookups" << std::endl;
  interval_list ilist;
  ilist.num_queries = num_queries;
  ilist.num_subreads_per_query = num_subreads_per_query;
  ilist.ptr = new uint32_t**[num_queries];
  for (unsigned int i = 0; i < num_queries; i++) {
    ilist.ptr[i] = new uint32_t*[num_subreads_per_query];
    for (unsigned int j = 0; j < num_subreads_per_query; j++) {
      ilist.ptr[i][j] = new uint32_t[2];
      assert(srlist.ptr[i][j] < interval_table.length - 1);
      ilist.ptr[i][j][0] = interval_table.ptr[srlist.ptr[i][j]];
      ilist.ptr[i][j][1] = interval_table.ptr[srlist.ptr[i][j] + 1];
    }
  }

  // Look up positions for each subread
  std::cout << "Performing position table lookups" << std::endl;
  std::ofstream results_file;
  results_file.open(argv[5]);
  results_file << num_queries << std::endl;
  for (unsigned int i = 0; i < num_queries; i++) {
    if (i % 10000 == 0) {
      std::cout << "Query " << i+1 << " out of " << num_queries << std::endl;
    }
    std::vector<unsigned int>* prev_result = new std::vector<unsigned int>(&(position_table.ptr[ilist.ptr[i][0][0]]), &(position_table.ptr[ilist.ptr[i][0][1]]));
    for (unsigned int j = 1; j < num_subreads_per_query; j++) {
      std::vector<unsigned int> next_positions (&(position_table.ptr[ilist.ptr[i][j][0]]), &(position_table.ptr[ilist.ptr[i][j][1]]));
      std::vector<unsigned int>* result = new std::vector<unsigned int>;
      if (prev_result->size() == 0) {
        break;
      }
      merge(prev_result, &next_positions, result, j*subread_length);
      delete prev_result;
      prev_result = result;      
    }
    
    std::vector<unsigned int>::iterator it;
    for (it = prev_result->begin(); it != prev_result->end(); it++) {
      results_file << *it << ' ';
    }
    results_file << std::endl;
    delete prev_result;
  }
  results_file.close();
}