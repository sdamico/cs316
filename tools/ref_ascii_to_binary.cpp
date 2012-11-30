
#include <iostream>
#include <fstream>
#include <cstdlib>

using namespace std;

int main (int argc, char* argv[]) {
  if (argc < 3) {
    cout << "Usage: " << argv[0] << " <Ref Seq Filename> <Output Filename> [ASCII Filename]" << endl;
    exit(1);
  }
  
  ifstream ref_seq_file;
  ref_seq_file.open(argv[1]);
  char tempbuf[256];
  ref_seq_file.getline(tempbuf, 256);
  unsigned int ref_seq_length = (atoi(tempbuf) >> 2) << 2;
  
  ofstream out_file;
  out_file.open(argv[2]);
  out_file.write((char *) &ref_seq_length, sizeof(unsigned int));
  
  char quad = 0;
  int char_num = 0;
  unsigned int i;
  for (i = 0; i < ref_seq_length; i++) {
    char nucleotide = ref_seq_file.get();
    switch (nucleotide) {
      case 'A': quad += 0; break;
      case 'C': quad += 1; break;
      case 'G': quad += 2; break;
      case 'T': quad += 3; break;
      default : break;
    }
    
    if (char_num == 3) {
      out_file.put(quad);
      quad = 0;
      char_num = 0;
    } else {
      quad <<= 2;
      char_num++;
    }
  }

  out_file.close();
  ref_seq_file.close();
  
  if (argc == 4) {
    ifstream read_file;
    read_file.open(argv[2]);
    read_file.read((char *) &ref_seq_length, sizeof(unsigned int));
    
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
}