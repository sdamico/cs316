CC=g++
CFLAGS = -g -Wall
CORE_DIR = ../core

all: benchmark

benchmark: benchmark.o
	mkdir -p bin/
	$(CC) $(CFLAGS) benchmark.o -o bin/benchmark

benchmark.o: benchmark.cpp
	$(CC) $(CFLAGS) -c benchmark.cpp -I $(CORE_DIR)

clean:
	rm -rf *.o bin
