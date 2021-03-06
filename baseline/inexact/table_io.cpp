// Provides routines to read in tables generated by the generation tools

#include <iostream>
#include <fstream>
#include "table_io.h"

/* Reads in the interval table from the given filename. Allocates the table
 * space at the given address and stores the contents.
 */
void ReadIntervalTable (char* filename, table* interval_table) {
  unsigned int interval_table_size;
  std::ifstream interval_table_file;
  interval_table_file.open(filename);
  interval_table_file.read((char *)(&interval_table_size), sizeof(unsigned int));
  interval_table->ptr = new unsigned int[interval_table_size];
  interval_table->length = interval_table_size;
  interval_table_file.read((char *)(interval_table->ptr), interval_table_size * sizeof(unsigned int));
  interval_table_file.close();
}

/* Reads in the position table from the given filename. Allocates the table
 * space at the given address and stores the contents.
 */
void ReadPositionTable (char* filename, table* position_table) {
  unsigned int ref_seq_length;
  unsigned int seed_length;
  std::ifstream position_table_file;
  position_table_file.open(filename);
  position_table_file.read((char *)(&ref_seq_length), sizeof(unsigned int));
  position_table_file.read((char *)(&seed_length), sizeof(unsigned int));
  position_table->ptr = new unsigned int[ref_seq_length - seed_length + 1];
  position_table->length = ref_seq_length - seed_length + 1;
  position_table_file.read((char *)(position_table->ptr), (ref_seq_length - seed_length + 1) * sizeof(unsigned int));
  position_table_file.close();
}