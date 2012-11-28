/* Generates two tables that provide a lookup between a seed sequence and
 * a list of positions in the reference sequence where the seed exists.
 *
 * The position table is an N-integer table, where N is the length of the
 * reference sequence, that stores the positions 0 to N-1, with positions
 * corresponding to the same seed sequence grouped together and sorted.
 * For example, the position table for the reference sequence TCGACGAT with
 * a 2-character seed length is [3 6 1 4 2 5 0].
 *
 * The interval table is a 4^k integer table, where k is the seed length, that
 * stores the index of the first position in the position table that corresponds
 * to each seed sequence. The interval table for the reference sequence TCGACGAT
 * with a 2-character seed length is [0 0 1 1 2 2 2 4 4 6 6 6 6 6 7 7].
 */

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <string>
#include <list>
#include <assert.h>
#include <stdint.h>

using namespace std;

/* Converts a nucleotide sequence to an integer with the following encoding:
 * A : 00b
 * C : 01b
 * G : 10b
 * T : 11b
 */
unsigned int seq2int(list<unsigned char>* seq) {
  unsigned int seq_int = 0;
  
  list<unsigned char>::iterator it;
  for (it = seq->begin(); it != seq->end(); it++) {
    seq_int <<= 2;
    seq_int += *it;
  }
  
  return seq_int;
}

int main (int argc, char* argv[]) {
  if (argc < 5 || argc == 6) {
    cout << "Usage: " << argv[0] << " <Ref Seq File> <Seed Length> <Interval Table Filename> <Position Table Filename> [ASCII Interval Table Filename] [ASCII Position Table Filename]" << endl;
    exit(1);
  }
  
  // Get the reference sequence length
  unsigned int ref_seq_length;
  ifstream ref_seq_file;
  ref_seq_file.open(argv[1]);
  ref_seq_file.read((char *)(&ref_seq_length), sizeof(unsigned int));
  
  // Generate linked-list-based position lookup table
  cout << "Computing linked-list-based position lookup table" << endl;
  unsigned int seed_length = (unsigned int) atoi(argv[2]);
  unsigned int num_seeds = 1 << (2 * seed_length);
  list<unsigned int>* seed2index = new list<unsigned int>[num_seeds];
  list<unsigned char>* cur_seed = new list<unsigned char>;
  
  unsigned char quad;
  unsigned int char_num;
  unsigned int cur_index = 0;
  for (unsigned int i = 0; i < ref_seq_length; i++) {
    if (i % 10000000 == 0) {
      std::cout << "Linked List: " << i << " out of " << ref_seq_length << std::endl;
    }
    if (i % 4 == 0) {
      quad = ref_seq_file.get();
      char_num = 0;
    }
    unsigned char nucleotide = (quad & (3 << (3-char_num)*2)) >> (3-char_num)*2;
    cur_seed->push_back(nucleotide);
    
    if (cur_seed->size() > seed_length) {
      cur_seed->pop_front();
    }
    if (cur_seed->size() == seed_length) {
      seed2index[seq2int(cur_seed)].push_back(cur_index);
      cur_index++;
    }
    
    char_num++;
  }
  assert(cur_index == ref_seq_length - seed_length + 1);
  ref_seq_file.close();
  
  // Generate interval table and position table
  cout << "Computing interval and position tables" << endl;
  unsigned int interval_table_size = num_seeds + 1;
  int* interval_table = new int[interval_table_size];
  int* position_table = new int[ref_seq_length - seed_length + 1];
  
  int start = 0, size = 0;
  int index = 0;
  for (unsigned int i = 0; i < num_seeds; i++) {
    if (i % 10000000 == 0) {
      std::cout << "Tables " << i << " out of " << num_seeds << std::endl;
    }
    while (!seed2index[i].empty()) {
      position_table[index] = seed2index[i].front();
      seed2index[i].pop_front();
      index++;
      size++;
    }
    interval_table[i] = start;
    start += size;
    size = 0;
  }
  interval_table[num_seeds] = ref_seq_length - seed_length + 1;
  assert(index == ref_seq_length - seed_length + 1);
  
  // Write tables to disk  
  cout << "Writing position table to disk" << endl;
  ofstream position_table_file;
  position_table_file.open(argv[4]);
  position_table_file.write((char *)(&ref_seq_length), sizeof(unsigned int));
  position_table_file.write((char *)(&seed_length), sizeof(unsigned int));
  position_table_file.write((char *)position_table, (ref_seq_length - seed_length + 1) * sizeof(int));
  position_table_file.close();
  
  cout << "Writing interval table to disk" << endl;
  ofstream interval_table_file;
  interval_table_file.open(argv[3]);
  interval_table_file.write((char *)(&interval_table_size), sizeof(unsigned int));
  interval_table_file.write((char *)interval_table, (num_seeds + 1) * sizeof(int));
  interval_table_file.close();

  if (argc == 7) {
    cout << "Writing ASCII position table to disk" << endl;
    ifstream position_table_file_read;
    position_table_file_read.open(argv[4]);
    position_table_file_read.read((char *)(&ref_seq_length), sizeof(unsigned int));
    position_table_file_read.read((char *)(&seed_length), sizeof(unsigned int));
    num_seeds = 1 << (2 * seed_length);
    
    int* position_table_read = new int[ref_seq_length - seed_length + 1];
    position_table_file_read.read((char *)position_table_read, (ref_seq_length - seed_length + 1) * sizeof(int));
    position_table_file_read.close();
    
    ofstream position_table_ascii_file;
    position_table_ascii_file.open(argv[6]);
    position_table_ascii_file << ref_seq_length << endl;
    position_table_ascii_file << seed_length << endl;
    for (int i = 0; i < ref_seq_length - seed_length + 1; i++) {
      position_table_ascii_file << position_table_read[i] << ' ';
    }
    position_table_ascii_file.close();
    
    cout << "Writing ASCII interval table to disk" << endl;
    ifstream interval_table_file_read;
    interval_table_file_read.open(argv[3]);
    interval_table_file_read.read((char *)(&interval_table_size), sizeof(unsigned int));
    
    int* interval_table_read = new int[interval_table_size];    
    interval_table_file_read.read((char *)interval_table_read, interval_table_size * sizeof(int));
    interval_table_file_read.close();
    
    ofstream interval_table_ascii_file;
    interval_table_ascii_file.open(argv[5]);
    interval_table_ascii_file << interval_table_size << endl;
    for (int i = 0; i < interval_table_size; i++) {
      interval_table_ascii_file << interval_table_read[i] << ' ';
    }
    interval_table_ascii_file.close();
  }
}