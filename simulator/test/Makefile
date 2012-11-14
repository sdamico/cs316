CC=g++
CFLAGS = -g -Wall
CORE_DIR = ../core

all: fifo

fifo: fifo_test.o
	mkdir -p bin/
	$(CC) $(CFLAGS) fifo_test.o -o bin/fifo_test

fifo_test.o: fifo_test.cpp
	$(CC) $(CFLAGS) -c fifo_test.cpp -I $(CORE_DIR)

clean:
	rm -rf *.o bin
