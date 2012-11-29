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
 *
 * NOTE: The program uses ~5 GB memory for seed length of 15 and ref length of 225M
 *       On a 12 GB machine, can't run more than seed length of 15.
 */

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <list>
#include <cmath>

/* Converts a nucleotide sequence to an integer with the following encoding:
 * A : 00b
 * C : 01b
 * G : 10b
 * T : 11b
 */
unsigned int seq2int(std::list<unsigned char>* seq) {
  unsigned int seq_int = 0;
  
  std::list<unsigned char>::iterator it;
  for (it = seq->begin(); it != seq->end(); it++) {
    seq_int <<= 2;
    seq_int += *it;
  }
  
  return seq_int;
}

int main (int argc , char** argv) {
  if (argc < 5) {
    std::cout << "Usage: " << argv[0] << " <Ref Seq Filename> <Seed Length (<=15)> <Interval Table Filename> <Position Table Filename> [ASCII Interval Table Filename] [ASCII Position Table Filename]" << std::endl;
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
  
  // Compute position table bin sizes
  std::cout << "Computing position table bin sizes" << std::endl;
  unsigned int seed_length = (unsigned int) atoi(argv[2]);
  unsigned int num_seeds = 1 << (2 * seed_length);
  unsigned int* bin_sizes = new unsigned int[num_seeds + 1];
  for (unsigned int i = 0; i < num_seeds + 1; i++) {
    bin_sizes[i] = 0;
  }
  std::list<unsigned char> cur_seed;
  unsigned char quad;
  unsigned int char_num;
  for (unsigned int i = 0; i < ref_seq_length; i++) {
    if (i % 10000000 == 0) {
      std::cout << "Bin sizes " << i << " out of " << ref_seq_length << std::endl;
    }
    if (i % 4 == 0) {
      quad = ref[i/4];
    }
    unsigned int char_num = i % 4;
    unsigned char nucleotide = (quad & (3 << (3-char_num)*2)) >> (3-char_num)*2;
    cur_seed.push_back(nucleotide);
    
    if (cur_seed.size() > seed_length) {
      cur_seed.pop_front();
    }
    
    if (cur_seed.size() == seed_length) {
      bin_sizes[seq2int(&cur_seed)]++;
    }
  }
  
  // Compute interval table
  std::cout << "Computing interval table" << std::endl;
  unsigned int next_bin = bin_sizes[0];
  bin_sizes[0] = 0;
  for (unsigned int i = 1; i < num_seeds; i++) {
    if (i % 10000000 == 0) {
      std::cout << "Interval table " << i << " out of " << num_seeds << std::endl;
    }
    unsigned int temp = bin_sizes[i];
    bin_sizes[i] = next_bin;
    next_bin = next_bin + temp;
  }
  bin_sizes[num_seeds] = next_bin;
  unsigned int* interval_table = bin_sizes;
  
  // Write interval table
  std::cout << "Writing interval table" << std::endl;
  std::ofstream interval_table_file(argv[3]);
  unsigned int interval_table_size = num_seeds + 1;
  interval_table_file.write((char *)(&interval_table_size), sizeof(unsigned int));
  interval_table_file.write((char *) interval_table, interval_table_size * sizeof(unsigned int));
  interval_table_file.close();

  // Translate interval table to ASCII
  if (argc >= 6) {
    std::cout << "Writing ASCII interval table" << std::endl;
    std::ifstream interval_table_file_read(argv[3]);
    std::ofstream interval_table_ascii_file(argv[5]);
    
    interval_table_file_read.read((char *)(&interval_table_size), sizeof(unsigned int));
    interval_table_ascii_file << interval_table_size << std::endl;
    
    unsigned int val;
    for (int i = 0; i < interval_table_size; i++) {
      interval_table_file_read.read((char *)(&val), sizeof(unsigned int));
      interval_table_ascii_file << (int) val << ' ';
    }
    interval_table_file_read.close();
    interval_table_ascii_file.close();
  }
  
  // Compute position table
  std::cout << "Computing position table" << std::endl;
  unsigned int position_table_length = ref_seq_length - seed_length + 1;
  unsigned int* position_table = new unsigned int[position_table_length];
  unsigned int* position_cntrs = interval_table;
  cur_seed.clear();
  unsigned int cur_index = 0;
  for (unsigned int i = 0; i < ref_seq_length; i++) {
    if (i % 10000000 == 0) {
      std::cout << "Position table " << i << " out of " << ref_seq_length << std::endl;
    }
    if (i % 4 == 0) {
      quad = ref[i/4];
    }
    unsigned int char_num = i % 4;
    unsigned char nucleotide = (quad & (3 << (3-char_num)*2)) >> (3-char_num)*2;
    cur_seed.push_back(nucleotide);
    
    if (cur_seed.size() > seed_length) {
      cur_seed.pop_front();
    }
    
    if (cur_seed.size() == seed_length) {
      unsigned int cur_seed_int = seq2int(&cur_seed);
      
      unsigned int cnt = position_cntrs[cur_seed_int];
      position_table[cnt] = cur_index;

      cur_index++;
      position_cntrs[cur_seed_int]++;
      
    }
  }

  // Write position table
  std::cout << "Writing position table" << std::endl;
  std::ofstream position_table_file(argv[4]);
  position_table_file.write((char *)(&ref_seq_length), sizeof(unsigned int));
  position_table_file.write((char *)(&seed_length), sizeof(unsigned int));
  position_table_file.write((char *)position_table, position_table_length * sizeof(unsigned int));
  position_table_file.close();
  
  // Translate position table to ASCII
  if (argc >= 7) {
    std::cout << "Writing ASCII position table" << std::endl;
    std::ifstream position_table_file_read(argv[4]);
    std::ofstream position_table_ascii_file(argv[6]);
    
    position_table_file_read.read((char *)(&ref_seq_length), sizeof(unsigned int));
    position_table_ascii_file << ref_seq_length << std::endl;
    
    position_table_file_read.read((char *)(&seed_length), sizeof(unsigned int));
    position_table_ascii_file << seed_length << std::endl;
    
    unsigned int val;
    for (int i = 0; i < position_table_length; i++) {
      position_table_file_read.read((char *)(&val), sizeof(unsigned int));
      position_table_ascii_file << val << ' ';
    }
    position_table_file_read.close();
    position_table_ascii_file.close();
  }
}