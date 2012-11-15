CC=g++
CFLAGS = -g -Wall
CORE_DIR = ../core

all: fifo ram ram_module

fifo: fifo_test.o
	mkdir -p bin/
	$(CC) $(CFLAGS) fifo_test.o -o bin/fifo_test

fifo_test.o: fifo_test.cpp
	$(CC) $(CFLAGS) -c fifo_test.cpp -I $(CORE_DIR)

ram: ram_test.o
	mkdir -p bin/
	$(CC) $(CFLAGS) ram_test.o -o bin/ram_test

ram_test.o: ram_test.cpp
	$(CC) $(CFLAGS) -c ram_test.cpp -I $(CORE_DIR)

ram_module: ram_module_test.o
	mkdir -p bin/
	$(CC) $(CFLAGS) ram_module_test.o -o bin/ram_module_test

ram_module_test.o: ram_module_test.cpp
	$(CC) $(CFLAGS) -c ram_module_test.cpp -I $(CORE_DIR)

clean:
	rm -rf *.o bin
