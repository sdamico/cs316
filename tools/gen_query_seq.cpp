/* Generates all query sequences of a certain length of a given reference sequence
 * and writes a query sequence file.
 * File format:
 *   Number of queries (4 bytes)
 *   Query length      (4 bytes)
 *   Query sequences   (2 bits per nucleotide, queries aligned on byte boundaries)
 *
 * NOTE: query_length must be greater than 5 or may segfault.
 */

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <string>
#include <list>
#include <time.h>
#include <cmath>

using namespace std;

void write_query (std::list<unsigned char>* query, std::ofstream* out_file) {
  int char_num = 0;
  char quad = 0;
  
  std::list<unsigned char>::iterator it;
  for (it = query->begin(); it != query->end(); it++) {
    quad += (*it) << (3-char_num)*2;
    if (char_num == 3) {
      out_file->write(&quad, sizeof(char));
      quad = 0;
    }
    char_num = (char_num + 1) % 4;
  }
  if (char_num != 0) {
    out_file->write(&quad, sizeof(char));
  }
}

int main (int argc, char* argv[]) {
  if (argc < 5) {
    cout << "Usage: " << argv[0] << " <Ref Seq File> <Query Seq Length> <Num Queries> <Output Filename> [ASCII Filename]" << endl;
    exit(1);
  }

  // Read the reference sequence
  std::cout << "Reading reference sequence" << std::endl;
  unsigned int ref_seq_length;
  std::ifstream ref_seq_file;
  ref_seq_file.open(argv[1]);
  ref_seq_file.read((char *)(&ref_seq_length), sizeof(unsigned int));
  unsigned int ref_seq_bytes = (unsigned int) ceil((float) ref_seq_length / 4);
  unsigned char* ref = new unsigned char[ref_seq_bytes];
  ref_seq_file.read((char *)ref, ref_seq_bytes * sizeof(unsigned char));
  ref_seq_file.close();
  
  // Get query parameters
  std::cout << "Getting query parameters" << std::endl;
  unsigned int query_length = (unsigned int) atoi(argv[2]);
  unsigned int bytes_per_query = (unsigned int) ceil((float) query_length / 4);
  unsigned int total_num_queries = ref_seq_length - query_length + 1;
  unsigned int num_queries = (unsigned int) atoi(argv[3]);
  if (num_queries > total_num_queries) {
    cout << "Number of queries must be less than " << total_num_queries << " for this reference." << endl;
    exit(1);
  }
  
  // Compute random query indices
  std::cout << "Computing random query indices" << std::endl;
  unsigned int* query_indices = new unsigned int[num_queries];
  srand(time(NULL));
  for (unsigned int i = 0; i < num_queries; i++) {
    query_indices[i] = rand() % total_num_queries;
  }

  // Grab queries and write to output file
  std::cout << "Finding queries and writing to output file" << std::endl;
  ofstream out_file;
  out_file.open(argv[4]);
  out_file.write((char *)(&num_queries), sizeof(unsigned int));
  out_file.write((char *)(&query_length), sizeof(unsigned int));
  for (unsigned int i = 0; i < num_queries; i++) {
    unsigned int ref_bin = query_indices[i] / 4;
    unsigned int ref_offset = query_indices[i] % 4;
    for (unsigned int j = ref_bin; j < ref_bin + bytes_per_query; j++) {
      unsigned char query_char = (ref[j] << (ref_offset*2)) + (ref[j + 1] >> (8 - ref_offset*2));
      out_file.write((char *) &(query_char), sizeof(unsigned char));
    }
  }
  out_file.close();
  
  // Write ASCII file
  if (argc > 5) {
    std::cout << "Writing ASCII file" << std::endl;
    ifstream read_file;
    read_file.open(argv[4]);
    read_file.read((char *)(&num_queries), sizeof(unsigned int));
    read_file.read((char *)(&query_length), sizeof(unsigned int));
    
    ofstream ascii_file;
    ascii_file.open(argv[5]);
    ascii_file << num_queries << endl;
    ascii_file << query_length << endl;
    
    unsigned char quad;
    unsigned int char_num;
    for (int i = 0; i < num_queries; i++) {
      for (int j = 0; j < query_length; j++) {
        if (j % 4 == 0) {
          quad = read_file.get();
          char_num = 0;
        }
        unsigned char nucleotide = (quad & (3 << (3-char_num)*2)) >> (3-char_num)*2;
        switch (nucleotide) {
          case 0 : ascii_file << 'A'; break;
          case 1 : ascii_file << 'C'; break;
          case 2 : ascii_file << 'G'; break;
          case 3 : ascii_file << 'T'; break;
          default : break;
        }
        char_num++;
      }
      ascii_file << endl;
    }
    ascii_file.close();
    read_file.close();
  }
  
  return 0;
}
