#ifndef _table_io_h
#define _table_io_h

struct table {
  unsigned int  length;
  unsigned int* ptr;
};

void ReadIntervalTable (char* filename, table* interval_table);
void ReadPositionTable (char* filename, table* position_table);

#endif
