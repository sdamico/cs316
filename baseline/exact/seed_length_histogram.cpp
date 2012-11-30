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
#include <cmath>
#include <tr1/unordered_map>

using namespace std;

/* Converts a nucleotide sequence to an integer with the following encoding:
 * A : 00b
 * C : 01b
 * G : 10b
 * T : 11b
 */
unsigned long long seq2int(string seq) {
  unsigned long long seq_int = 0;
  
  string::iterator it;
  for (it = seq.begin(); it != seq.end(); it++) {
    seq_int <<= 2;
    switch (*it) {
      case 'A' : seq_int += 0; break;
      case 'C' : seq_int += 1; break;
      case 'G' : seq_int += 2; break;
      case 'T' : seq_int += 3; break;
      default : break;
    }
  }
  return seq_int;
}

int main (int argc, char* argv[]) {
  if (argc < 3) {
    cout << "Usage: " << argv[0] << " <Ref Seq File> <Seed Length>" << endl;
    exit(1);
  }
  
  // Get the reference sequence length
  unsigned int ref_seq_length;
  ifstream ref_seq_file;
  ref_seq_file.open(argv[1]);
  
  char temp_buf[256];
  ref_seq_file.getline(temp_buf, 256);
  ref_seq_length = atoi(temp_buf);
  
  // Compute number of occurrences for each seed
  cout << "Computing seed occurrences" << endl;
  unsigned int seed_length = (unsigned int) atoi(argv[2]);
  unsigned int num_seeds = 1 << (2 * seed_length);
  
  tr1::unordered_map<unsigned long long, int> seed_count;
  
  string cur_seed;
  unsigned char quad;
  unsigned int char_num;
  unsigned key_count = 0;
  for (unsigned int i = 0; i < ref_seq_length; i++) {
    if (i % 10000000 == 0) {
      cout << i << endl;
    }
//    if (i % 4 == 0) {
//      quad = ref_seq_file.get();
//      char_num = 0;
//    }
//    unsigned char nucleotide = (quad & (3 << (3-char_num)*2)) >> (3-char_num)*2;
    unsigned char nucleotide = ref_seq_file.get();
    cur_seed.push_back(nucleotide);
    
    if (cur_seed.size() > seed_length) {
      cur_seed = cur_seed.substr(1, seed_length);
    }
    if (cur_seed.size() == seed_length) {
      unsigned long long seed = seq2int(cur_seed);
      if (seed_count.count(seed) == 0) {
        seed_count[seed] = 1;
        key_count++;
      } else {
        seed_count[seed]++;
      }
    }
    char_num++;
  }
  ref_seq_file.close();
  
  // Compute histogram
  cout << "Computing histogram" << endl;
  const int NUM_BINS = 25;
  unsigned int hist_seed[NUM_BINS];
  unsigned int hist_read[NUM_BINS];
  for (int i = 0; i < NUM_BINS; i++) {
    hist_seed[i] = 0;
    hist_read[i] = 0;
  }
  unsigned int hist_low[NUM_BINS];
  unsigned int hist_high[NUM_BINS];
  
  for (int i = 0; i < 11; i++) {
    hist_high[i] = i;
    hist_low[i] = i;
  }
  for (int i = 11; i < NUM_BINS; i++) {
    hist_high[i] = pow(10.0, i-9.0);
    hist_low[i] = pow(10.0, i-10.0) + 1;
  }
  
  key_count = 0;
  for (tr1::unordered_map<unsigned long long, int>::iterator it = seed_count.begin(); it != seed_count.end(); it++) {
    key_count++;
    unsigned int count = it->second;
    for (int j = 0; j < NUM_BINS; j++) {
      if (count >= hist_low[j] && count <= hist_high[j]) {
        hist_seed[j]++;
        hist_read[j] += count;
        continue;
      }
    }
  }

  for (int i = 0; i < NUM_BINS; i++) {
    if (hist_low[i] == hist_high[i]) {
      cout << hist_low[i] << "\t";
    } else {
      cout << hist_low[i] << "-" << hist_high[i] << "\t";
    }
    cout << hist_seed[i] << "\t" << hist_read[i] << endl;
  }  
}
