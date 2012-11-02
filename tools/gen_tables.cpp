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

#define DEBUG

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <string>
#include <list>
#include <assert.h>

using namespace std;

/* Converts a nucleotide sequence to an integer with the following encoding:
 * A : 00b
 * C : 01b
 * G : 10b
 * T : 11b
 */
int seq2int(string seq) {
  int seq_int = 0;
  
  string::iterator it;
  for (it = seq.begin(); it < seq.end(); it++) {
    seq_int <<= 2;
    switch(*it) {
      case 'A' : seq_int += 0; break;
      case 'C' : seq_int += 1; break;
      case 'G' : seq_int += 2; break;
      case 'T' : seq_int += 3; break;
      default  : break;
    }
  }
  
  return seq_int;
}

int main (int argc, char* argv[]) {
  if (argc < 5) {
    cout << "Usage: " << argv[0] << " <Ref Seq File> <Seed Length> <Interval Table File> <Position Table File>" << endl;
    exit(1);
  }
  
  // Get the reference sequence length
  ifstream ref_seq_file(argv[1]);
  char* ref_seq_length_str = new char[256];
  ref_seq_file.getline(ref_seq_length_str, 256);
  int ref_seq_length = atoi(ref_seq_length_str);
  
  // Generate linked-list-based position lookup table
  cout << "Computing linked-list-based position lookup table" << endl;
  int seed_length = atoi(argv[2]);
  int num_seeds = 1 << (2 * seed_length);
  cout << seed_length << '\t' << num_seeds << endl;
  list<int>* seed2index = new list<int>[num_seeds];
  
  string cur_seed = " ";
  for (int i = 0; i < seed_length - 1; i++) {
    cur_seed += ref_seq_file.get();
  }
  
  int cur_index = 0;
  while (ref_seq_file.good()) {
    char c = ref_seq_file.get();
    if (c == 'A' || c == 'C' || c == 'G' || c == 'T') {
      cur_seed += c;
      cur_seed.erase(0, 1);
      seed2index[seq2int(cur_seed)].push_back(cur_index);
      cur_index++;
    }
  }
  assert(cur_index == ref_seq_length - seed_length + 1);
  ref_seq_file.close();
  
  // Generate interval table and position table
  cout << "Computing interval and position tables" << endl;
  int* interval_table = new int[num_seeds];
  int* position_table = new int[ref_seq_length - seed_length + 1];
  
  int start = 0, size = 0;
  int index = 0;
  for (int i = 0; i < num_seeds; i++) {
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
  assert(index == ref_seq_length - seed_length + 1);
  
  // Write tables to disk  
  cout << "Writing position table to disk" << endl;
  ofstream position_table_file;
  position_table_file.open(argv[4]);
  position_table_file.write((char *)position_table, (ref_seq_length - seed_length + 1) * sizeof(int));
  position_table_file.close();
  
  cout << "Writing interval table to disk" << endl;
  ofstream interval_table_file;
  interval_table_file.open(argv[3]);
  interval_table_file.write((char *)interval_table, num_seeds * sizeof(int));
  interval_table_file.close();

#ifdef DEBUG
  int* position_table_test = new int[ref_seq_length - seed_length + 1];
  int* interval_table_test = new int[num_seeds];
  
  ifstream position_table_file_test;
  position_table_file_test.open(argv[4]);
  position_table_file_test.read((char *)position_table_test, (ref_seq_length - seed_length + 1) * sizeof(int));
  position_table_file_test.close();
  
  ifstream interval_table_file_test;
  interval_table_file_test.open(argv[3]);
  interval_table_file_test.read((char *)interval_table_test, num_seeds * sizeof(int));
  interval_table_file.close();
  
  for (int i = 0; i < ref_seq_length - seed_length + 1; i++) {
    assert(position_table_test[i] == position_table[i]);
  }
  
  for (int i = 0; i < num_seeds; i++) {
    assert(interval_table_test[i] == interval_table[i]);
  }
  
  cout << "File verification complete" << endl;
#endif
}