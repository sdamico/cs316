CC=g++
CFLAGS = -g -Wall

all: baseline

baseline: main.o table_io.o
	mkdir -p bin/
	$(CC) $(CFLAGS) main.o table_io.o -o bin/baseline

main.o: main.cpp
	$(CC) $(CFLAGS) -c main.cpp

table_io.o: table_io.cpp
	$(CC) $(CFLAGS) -c table_io.cpp

clean:
	rm -rf *.o bin/baseline  
