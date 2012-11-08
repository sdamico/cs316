#ifndef _def_h
#define _def_h

#include <stdint.h>

struct query_list {
  int num_queries;
  int query_length;
  unsigned char** ptr;
};

struct subread_list {
  int num_queries;
  int num_subreads_per_query;
  uint32_t** ptr;
};

struct interval_list {
  int num_queries;
  int num_subreads_per_query;
  uint32_t*** ptr;
};

#endif
