/* Generates all subreads of the query sequence file and writes to
 * a subread file.
 * File format:
 *   Number of queries            (4 bytes)
 *   Number of subreads per query (4 bytes)
 *   Subread length               (4 bytes)
 *   Subread sequences            (64-bits per subread, left zero-padded)
 */

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cmath>

int main (int argc, char** argv) {
  if (argc < 4) {
    std::cout << "Usage: " << argv[0] << " <Subread Length> <Queries Filename> <Output Filename> [ASCII Filename]" << std::endl;
    exit(1);
  }
  
  if (atoi(argv[1]) > 32) {
    std::cout << "Subread length must be less than 32." << std::endl;
    exit(1);
  }
  
  std::ifstream queries_file;
  unsigned int num_queries;
  unsigned int query_length;
  queries_file.open(argv[2]);
  queries_file.read((char *)(&num_queries), sizeof(unsigned int));
  queries_file.read((char *)(&query_length), sizeof(unsigned int));
  
  unsigned int subread_length = atoi(argv[1]);
  // Truncating partial subreads
  unsigned int num_subreads_per_query = query_length / subread_length;
  
  // Read in query list
  unsigned int bytes_per_query = (unsigned int) ceil((float)query_length /4);
  unsigned char** queries = new unsigned char*[num_queries];
  for (unsigned int i = 0; i < num_queries; i++) {
    queries[i] = new unsigned char[bytes_per_query];
    queries_file.read((char *) (queries[i]), bytes_per_query * sizeof(unsigned char));
  }
  
  // Split query list into subread list
  uint64_t** subreads = new uint64_t*[num_queries];
  for (unsigned int i = 0; i < num_queries; i++) {
    subreads[i] = new uint64_t[num_subreads_per_query];
  }
  for (unsigned int i = 0; i < num_queries; i++) {
    unsigned int bit_index = 0;
    for (unsigned int j = 0; j < num_subreads_per_query; j++) {
      unsigned int byte = bit_index / 8;
      unsigned int offset = bit_index % 8;
      uint64_t subread = 0;
      subread += queries[i][byte] & (0xFF >> offset);
      unsigned int bits_read = 8 - offset;
      byte++;
      while ((bits_read < subread_length*2) && (subread_length*2 - bits_read >= 8)) {
        subread <<= 8;
        subread += queries[i][byte];
        byte++;
        bits_read += 8;
      }
      if (subread_length*2 - bits_read > 0) {
        subread <<= (subread_length*2 - bits_read);
        subread += queries[i][byte] >> (8 - (subread_length*2 - bits_read));
        bits_read += (subread_length*2 - bits_read);
      }
      subreads[i][j] = subread;
      bit_index += (subread_length*2);
    }
  }
  
  // Write subread list into file
  std::ofstream out_file;
  out_file.open(argv[3]);
  out_file.write((char *)(&num_queries), sizeof(unsigned int));
  out_file.write((char *)(&num_subreads_per_query), sizeof(unsigned int));
  out_file.write((char *)(&subread_length), sizeof(unsigned int));
  for (unsigned int i = 0; i < num_queries; i++) {
    for (unsigned int j = 0; j < num_subreads_per_query; j++) {
      out_file.write((char *)(&(subreads[i][j])), sizeof(uint64_t));
    }
  }
  out_file.close();
  
  // Write subread ASCII file
  if (argc == 5) {
    std::ifstream read_file;
    read_file.open(argv[3]);
    read_file.read((char *)(&num_queries), sizeof(unsigned int));
    read_file.read((char *)(&num_subreads_per_query), sizeof(unsigned int));
    read_file.read((char *)(&subread_length), sizeof(unsigned int));
    
    std::ofstream ascii_file;
    ascii_file.open(argv[4]);
    ascii_file << num_queries << std::endl;
    ascii_file << num_subreads_per_query << std::endl;
    ascii_file << subread_length << std::endl;
    
    for (int i = 0; i < num_queries; i++) {
      for (int j = 0; j < num_subreads_per_query; j++) {
        uint64_t subread;
        read_file.read((char *)(&subread), sizeof(uint64_t));
        for (int k = 0; k < subread_length; k++) {
          uint64_t nucleotide = (subread & (3 << (subread_length - k - 1) * 2)) >> (subread_length - k - 1) * 2;
          switch (nucleotide) {
            case 0 : ascii_file << 'A'; break;
            case 1 : ascii_file << 'C'; break;
            case 2 : ascii_file << 'G'; break;
            case 3 : ascii_file << 'T'; break;
            default : ascii_file << 'X'; break;
          }
        }
        ascii_file << std::endl;
      }
    }
    ascii_file.close();
    read_file.close();
  }
  
  return 0;
}