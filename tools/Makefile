CC=g++
CFLAGS = -g -Wall

all: gen_query_seq gen_ref_seq gen_tables gen_query_error_SNP gen_subread_seq

gen_query_seq: gen_query_seq.o
	mkdir -p bin/
	$(CC) $(CFLAGS) gen_query_seq.o -o bin/gen_query_seq

gen_ref_seq: gen_ref_seq.o
	mkdir -p bin/
	$(CC) $(CFLAGS) gen_ref_seq.o -o bin/gen_ref_seq

gen_tables: gen_tables.o
	mkdir -p bin/
	$(CC) $(CFLAGS) gen_tables.o -o bin/gen_tables

gen_query_error_SNP: gen_query_error_SNP.o
	mkdir -p bin/
	$(CC) $(CFLAGS) gen_query_error_SNP.o -o bin/gen_query_error_SNP

gen_subread_seq: gen_subread_seq.o
	mkdir -p bin/
	$(CC) $(CFLAGS) gen_subread_seq.o -o bin/gen_subread_seq

clean:
	rm -rf *.o bin/
