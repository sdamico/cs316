#include <iostream>
#include <fstream>
#include <cstdlib>
#include <list>
#include <cmath>
#include <vector>
#include <time.h>
#include <stdint.h>

int main (int argc, char* argv[]) {
  if (argc < 3) {
    std::cout << "Usage: " << argv[0] << " <Query File> <Output File>" << std::endl;
    exit(1);
  }

  std::ifstream queries_file;
  unsigned int num_queries;
  unsigned int query_length;
  queries_file.open(argv[1]);
  queries_file.read((char *)(&num_queries), sizeof(unsigned int));
  queries_file.read((char *)(&query_length), sizeof(unsigned int));
  
  // Read in query list
  std::cout<<"Reading in query list"<<std::endl;
  unsigned char** qlist;
  unsigned int bytes_per_query = (unsigned int) ceil((float)query_length/4);
  qlist = new unsigned char*[num_queries];
  for (unsigned int i = 0; i < num_queries; i++) {
    qlist[i] = new unsigned char[bytes_per_query];
    queries_file.read((char *)(qlist[i]), bytes_per_query * sizeof(unsigned char));
  }
  queries_file.close();

  unsigned int* indices = new unsigned int[num_queries];
  for (unsigned int i = 0; i < num_queries; i++) {
    indices[i] = i;
  }
  
  std::cout<<"Introducing random swaps"<<std::endl;
  srand(time(NULL));
  for (unsigned int i = 0; i < num_queries; i++) {
    if (i % 1000000 == 0) {
      std::cout << "Swap " << i << " out of " << num_queries << std::endl;
    }
    int index1 = rand() % num_queries;
    int index2 = rand() % num_queries;
    unsigned char* temp = qlist[index1];
    qlist[index1] = qlist[index2];
    qlist[index2] = temp;
  }
  
  std::cout<<"Writing result"<<std::endl;
  std::ofstream out_file;
  out_file.open(argv[2]);
  out_file.write((char *)(&num_queries), sizeof(unsigned int));
  out_file.write((char *)(&query_length), sizeof(unsigned int));
  for (unsigned int i = 0; i < num_queries; i++) {
    out_file.write((char *)(qlist[i]), bytes_per_query * sizeof(unsigned char));
  }
  out_file.close();
}