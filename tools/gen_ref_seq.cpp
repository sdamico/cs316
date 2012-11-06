/* Generates a random reference sequence and writes a reference sequence file.
 * File format:
 *   Reference sequence length (4 bytes)
 *   Reference sequence        (2 bits per nucleotide)
 */

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <time.h>

using namespace std;

int main (int argc, char* argv[]) {
  if (argc < 3) {
    cout << "Usage: " << argv[0] << " <Ref Seq Length> <Output Filename> [ASCII Filename]" << endl;
    exit(1);
  }
  
  unsigned int ref_seq_length = atoi(argv[1]);
  
  ofstream out_file;
  out_file.open(argv[2]);
  out_file.write((char *)(&ref_seq_length), sizeof(unsigned int));
    
  srand(time(NULL));
  char quad;
  int char_num = 0;
  for (unsigned int i = 0; i < ref_seq_length; i++) {
    if (i % 4 == 0) {
      quad = 0;
      char_num = 0;
    }
    quad += (rand() % 4) << (3-char_num)*2;
    if (i % 4 == 3) {
      out_file.write(&quad, sizeof(char));
    }
    char_num++;
  }
  if (char_num != 4) {
    out_file.write(&quad, sizeof(char));
  }
  out_file.close();
  
  if (argc == 4) {
    ifstream read_file;
    read_file.open(argv[2]);
    read_file.read((char *)(&ref_seq_length), sizeof(unsigned int));
    
    ofstream ascii_file;
    ascii_file.open(argv[3]);
    ascii_file << ref_seq_length << endl;
    
    for (unsigned int i = 0; i < ref_seq_length; i++) {
      if (i % 4 == 0) {
        quad = read_file.get();
        char_num = 0;
      }
      char nucleotide = (quad & (3 << (3-char_num)*2)) >> (3-char_num)*2;
      switch (nucleotide) {
        case 0 : ascii_file << 'A'; break;
        case 1 : ascii_file << 'C'; break;
        case 2 : ascii_file << 'G'; break;
        case 3 : ascii_file << 'T'; break;
        default : break;
      }
      char_num++;
    }
    ascii_file.close();
    read_file.close();
  }
  
  return 0;
}
