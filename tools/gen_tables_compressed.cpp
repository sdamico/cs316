/* Generates two tables that provide a lookup between a seed sequence and
 * a list of positions in the reference sequence where the seed exists.
 *
 * The position table is an N-integer table, where N is the length of the
 * reference sequence, that stores the positions 0 to N-1, with positions
 * corresponding to the same seed sequence grouped together and sorted.
 * For example, the position table for the reference sequence TCGACGAT with
 * a 2-character seed length is [3 6 1 4 2 5 0].
 *
 * The interval table is a 4^k integer table, where k is the seed length, that
 * stores the index of the first position in the position table that corresponds
 * to each seed sequence. The interval table for the reference sequence TCGACGAT
 * with a 2-character seed length is [0 0 1 1 2 2 2 4 4 6 6 6 6 6 7 7].
 */

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <string>
#include <list>
#include <assert.h>
#include <stdint.h>

using namespace std;

//dead simple: write offset w/4bytes and deltas w/1byte, if a delta is longer than 1 byte, write a 0 and then write a new offset, continue.
//	returns a pointer to the next free location in outputstream and outputs the number of bytes in the compressed stream
void* compress1(unsigned int *inputStream, unsigned int inputLength, unsigned char *outputStream, unsigned int *compressedSize) {
	unsigned int offset = inputStream[0];
	if (inputLength != 0) {
		/*cout << "Uncompressed PTE:\t";
		for (unsigned int i = 0; i < inputLength; i++) {
			cout << inputStream[i] << "\t";
		}*/
		//cout << endl << "  Compressed PTE:\t";
		unsigned char *curOutput = outputStream;
		memcpy((void *)curOutput, (const void *)&offset, sizeof(unsigned int));
		//cout << offset << "w\t";
		curOutput += sizeof(unsigned int);
		for (unsigned int i = 1; i < inputLength; i++) {
			unsigned char delta;
			if ( (inputStream[i]-offset) > ((1 << 8*sizeof(unsigned char))-1) ) {
				delta = 0;
				memcpy((void *)curOutput, (const void *)&delta, sizeof(unsigned char));
				curOutput += sizeof(unsigned char);
	//			cout << (int)delta << "b\t";
				offset = inputStream[i];			
				memcpy((void *)curOutput, (const void *)&offset, sizeof(unsigned int));
				curOutput += sizeof(unsigned int);
	//			cout << offset << "w\t";
			}
			else {
				delta = inputStream[i] - offset;
	//			cout << (int)delta << "b\t";
				memcpy((void *)curOutput, (const void *)&delta, sizeof(unsigned char));
				curOutput += sizeof(unsigned char);
			}	
		}	
		//cout << endl;
		*compressedSize = curOutput - outputStream;
		return curOutput;
	}
	else {
		*compressedSize = 0;
		return outputStream;
	}	
}

int main (int argc, char* argv[]) {
  if (argc < 6) {
    cout << "Usage: " << argv[0] << " <Interval Table Filename In> <Position Table Filename In> <Interval Table Filename Out> <Position Table Filename Out>  <Compression Type>" << endl;
    exit(1);
  }
  
  //compression
  int compressionType = atoi(argv[5]);
  typedef void * (*CompressionFnPtr)(unsigned int *, unsigned int, unsigned char *, unsigned int *);
  CompressionFnPtr compressionFunctions [] = {compress1};
  CompressionFnPtr compressionFunction = compressionFunctions[compressionType];

  //read input files
  ifstream pt_file;
  unsigned int ref_seq_length, seed_length, num_seeds, pt_size;
  pt_file.open(argv[2]);
  pt_file.read((char*)(&ref_seq_length), sizeof(unsigned int));
  pt_file.read((char*)(&seed_length), sizeof(unsigned int));
  num_seeds = 1 << (2 * seed_length);
  pt_size = ref_seq_length - seed_length + 1;
  
  ifstream it_file;
  unsigned int it_size;
  it_file.open(argv[1]);
  it_file.read((char*)(&it_size), sizeof(unsigned int));
  
  //open compressed files and write headers
  ofstream ptc_file;
  ptc_file.open(argv[4]);
  ptc_file.write((const char*)(&ref_seq_length), sizeof(unsigned int));
  ptc_file.write((const char*)(&seed_length), sizeof(unsigned int));  
  
  //open compressed files and write headers
  ofstream itc_file;
  itc_file.open(argv[3]);
  itc_file.write((const char *)(&it_size),sizeof(unsigned int));
  
  unsigned int next_idx=0;
  unsigned int cur_idx=0;  
  unsigned int compressed_bytes_written = 0;
  unsigned int current_it_idx = 0;
  unsigned int max_pt_entry_size = 100;
  unsigned char *compress_buffer = new unsigned char[max_pt_entry_size*2*sizeof(unsigned int)];
  unsigned int *pt_buffer = new unsigned int[max_pt_entry_size];
  it_file.read((char*)(&cur_idx), sizeof(unsigned int));
  for (unsigned int i = 0; i < num_seeds-1; i++ ) {
	//read next interval table index to get size of this position table entry
    it_file.read((char*)(&next_idx), sizeof(unsigned int));
	unsigned int pt_entry_size = next_idx - cur_idx;
	//re-allocate our buffers in case this entry is bigger than the biggest one we've seen so far
	if (pt_entry_size > max_pt_entry_size) {
		delete [] compress_buffer;
		compress_buffer = new unsigned char[pt_entry_size*2*sizeof(unsigned int)];
		delete [] pt_buffer;
		pt_buffer = new unsigned int[pt_entry_size];
		max_pt_entry_size = pt_entry_size;
	}
	//read in position table entry 
	pt_file.read((char *)pt_buffer, sizeof(unsigned int)*pt_entry_size);
	//compress it
	compressionFunction(pt_buffer, pt_entry_size, compress_buffer, &compressed_bytes_written);
	//write interval table
	itc_file.write((const char *)&current_it_idx,sizeof(unsigned int));
	current_it_idx += compressed_bytes_written;
	//write position table compressed stream 
	ptc_file.write((const char *)compress_buffer, compressed_bytes_written);
	cur_idx = next_idx;
  }  
  //cout << endl;
  //write last itc entry
  itc_file.write((const char *)&current_it_idx,sizeof(unsigned int));
  
  pt_file.close();
  it_file.close();

  ptc_file.close();
  itc_file.close();
}
