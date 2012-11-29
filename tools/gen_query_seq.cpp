/* Generates all query sequences of a certain length of a given reference sequence
 * and writes a query sequence file.
 * File format:
 *   Number of queries (4 bytes)
 *   Query length      (4 bytes)
 *   Query sequences   (2 bits per nucleotide, queries aligned on byte boundaries)
 */

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <string>
#include <list>
#include <time.h>

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
  if (argc < 4) {
    cout << "Usage: " << argv[0] << " <Ref Seq File> <Query Seq Length> <Output Filename> [ASCII Filename] [Num Queries]" << endl;
    exit(1);
  }

  unsigned int ref_seq_length;
  
  ifstream ref_seq_file;
  ref_seq_file.open(argv[1]);
  ref_seq_file.read((char *)(&ref_seq_length), sizeof(unsigned int));
    
  unsigned int query_length = (unsigned int) atoi(argv[2]);
  unsigned int total_num_queries = ref_seq_length - query_length + 1;

  unsigned int num_queries;
  bool* query_mask = new bool[total_num_queries];
  for (unsigned int i = 0; i < total_num_queries; i++) {
    query_mask[i] = false;
  }
  if (argc >= 6) {
    num_queries = atoi(argv[5]);
    srand(time(NULL));
    unsigned int num_set = 0;
    while (num_set < num_queries) {
      int rand_num = rand() % total_num_queries;
      if (query_mask[rand_num] == false) {
        query_mask[rand_num] = true;
        num_set++;
      }
    }
  } else {
    num_queries = total_num_queries;
    for (unsigned int i = 0; i < total_num_queries; i++) {
      query_mask[i] = true;
    }
  }

  ofstream out_file;
  out_file.open(argv[3]);
  out_file.write((char *)(&num_queries), sizeof(unsigned int));
  out_file.write((char *)(&query_length), sizeof(unsigned int));
  
  list<unsigned char>* query = new list<unsigned char>;
  unsigned char quad;
  int char_num = 0;
  for (unsigned int i = 0; i < ref_seq_length; i++) {
    if (i % 1000000 == 0) {
      std::cout << i << std::endl;
    }
    if (i % 4 == 0) {
      quad = ref_seq_file.get();
      char_num = 0;
    }
    unsigned char nucleotide = (quad & (3 << (3-char_num)*2)) >> (3-char_num)*2;    
    
    query->push_back(nucleotide);
    if (query->size() > query_length) {
      query->pop_front();
    }
    if (query->size() == query_length && query_mask[i] == true) {
      write_query(query, &out_file);
    }
    
    char_num++;
  }
  
  ref_seq_file.close();
  out_file.close();
  
  if (argc >= 5) {
    ifstream read_file;
    read_file.open(argv[3]);
    read_file.read((char *)(&num_queries), sizeof(unsigned int));
    read_file.read((char *)(&query_length), sizeof(unsigned int));
    
    ofstream ascii_file;
    ascii_file.open(argv[4]);
    ascii_file << num_queries << endl;
    ascii_file << query_length << endl;
    
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
