CC=g++
CFLAGS = -g -Wall
CORE_DIR = ../core

all: fifo ram ram_module input_reader interval_table_ctrl position_table_ctrl stitcher

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

input_reader: input_reader_test.o
	mkdir -p bin/
	$(CC) $(CFLAGS) input_reader_test.o -o bin/input_reader_test

input_reader_test.o: input_reader_test.cpp
	$(CC) $(CFLAGS) -c input_reader_test.cpp -I $(CORE_DIR)

interval_table_ctrl: interval_table_ctrl_test.o
	mkdir -p bin/
	$(CC) $(CFLAGS) interval_table_ctrl_test.o -o bin/interval_table_ctrl_test

position_table_ctrl: position_table_ctrl_test.o
	mkdir -p bin/
	$(CC) $(CFLAGS) position_table_ctrl_test.o -o bin/position_table_ctrl_test

interval_table_ctrl_test.o: interval_table_ctrl_test.cpp
	$(CC) $(CFLAGS) -c interval_table_ctrl_test.cpp -I $(CORE_DIR)

position_table_ctrl_test.o: position_table_ctrl_test.cpp
	$(CC) $(CFLAGS) -c position_table_ctrl_test.cpp -I $(CORE_DIR)

stitcher: stitcher_test.o
	mkdir -p bin/
	$(CC) $(CFLAGS) stitcher_test.o -o bin/stitcher_test

stitcher_test.o: stitcher_test.cpp ../core/stitcher.h
	$(CC) $(CFLAGS) -c stitcher_test.cpp -I $(CORE_DIR)

clean:
	rm -rf *.o bin
