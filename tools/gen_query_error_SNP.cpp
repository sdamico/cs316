/* Reads in a query sequence file and introduces SNPs at a given error rate.
 */

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <time.h>
#include <cmath>

int main (int argc, char* argv[]) {
  if (argc < 4) {
    std::cout << "Usage: " << argv[0] << " <Query File> <SNP Rate (%)> <Output Filename> [ASCII Filename]" << std::endl;
    exit(1);
  }
  
  std::ifstream queries_file;
  unsigned int num_queries;
  unsigned int query_length;
  queries_file.open(argv[1]);
  queries_file.read((char *)(&num_queries), sizeof(unsigned int));
  queries_file.read((char *)(&query_length), sizeof(unsigned int));
  
  std::ofstream out_file;
  out_file.open(argv[3]);
  out_file.write((char *)(&num_queries), sizeof(unsigned int));
  out_file.write((char *)(&query_length), sizeof(unsigned int));
  
  srand(time(NULL));
  unsigned int bytes_per_query = (unsigned int) ceil((float)query_length/4);
  unsigned char* query = new unsigned char[bytes_per_query];
  double snp_rate = atof(argv[2]) / 100.0;
  for (unsigned int i = 0; i < num_queries; i++) {
    for (unsigned int j = 0; j < bytes_per_query; j++) {
      unsigned char quad = queries_file.get();

      for (unsigned int k = 0; k < 4; k++) {
        unsigned char nucleotide = (quad & (3 << (3-k)*2)) >> (3-k)*2;
        
        double r = (double)rand() / (double)RAND_MAX;
        if (r < snp_rate/4) {
          nucleotide = 0;
        } else if (r < snp_rate/2) {
          nucleotide = 1;
        } else if (r < 3*snp_rate/4) {
          nucleotide = 2;
        } else if (r < snp_rate) {
          nucleotide = 3;
        }
        
        quad = (quad & ~(3 << (3-k)*2)) | (nucleotide << (3-k)*2);
      }
      out_file.write((char *)(&quad), sizeof(unsigned char));
    }
  }
  out_file.close();
  
  if (argc == 5) {
    std::ifstream read_file;
    read_file.open(argv[3]);
    read_file.read((char *)(&num_queries), sizeof(unsigned int));
    read_file.read((char *)(&query_length), sizeof(unsigned int));
    
    std::ofstream ascii_file;
    ascii_file.open(argv[4]);
    ascii_file << num_queries << std::endl;
    ascii_file << query_length << std::endl;
    
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
          default : ascii_file << 'X'; break;
        }
        char_num++;
      }
      ascii_file << std::endl;
    }
    ascii_file.close();
    read_file.close();
  }
  
  return 0;
}