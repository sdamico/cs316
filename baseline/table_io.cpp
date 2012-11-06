// Provides routines to read in tables generated by the generation tools

#include <iostream>
#include <fstream>
#include "table_io.h"

/* Reads in the interval table from the given filename. Allocates the table
 * space at the given address and stores the contents.
 */
void ReadIntervalTable (char* filename, int* interval_table) {
  unsigned int num_seeds;
  std::ifstream interval_table_file;
  interval_table_file.open(filename);
  interval_table_file.read((char *)(&num_seeds), sizeof(unsigned int));
  interval_table = new int[num_seeds];
  interval_table_file.read((char *)interval_table, num_seeds * sizeof(int));
  interval_table_file.close();
}

/* Reads in the position table from the given filename. Allocates the table
 * space at the given address and stores the contents.
 */
void ReadPositionTable (char* filename, int* position_table) {
  unsigned int ref_seq_length;
  unsigned int seed_length;
  std::ifstream position_table_file;
  position_table_file.open(filename);
  position_table_file.read((char *)(&ref_seq_length), sizeof(unsigned int));
  position_table_file.read((char *)(&seed_length), sizeof(unsigned int));
  position_table = new int[ref_seq_length - seed_length + 1];
  position_table_file.read((char *)position_table, (ref_seq_length - seed_length + 1) * sizeof(int));
  position_table_file.close();
}