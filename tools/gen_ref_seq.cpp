// Generates a random reference sequence

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <time.h>

using namespace std;

int main (int argc, char* argv[]) {
    if (argc < 3) {
        cout << "Usage: " << argv[0] << " <Ref Seq Length> <Output Filename>" << endl;
        exit(1);
    }

    ofstream out_file;
    out_file.open(argv[2]);
    out_file << argv[1] << endl;
    
    srand(time(NULL));
    for (int i = 0; i < atoi(argv[1]); i++) {
        int rand_num = rand() % 4;
        switch(rand_num) {
            case 0 : out_file << "A";
                     break;
            case 1 : out_file << "C";
                     break;
            case 2 : out_file << "G";
                     break;
            case 3 : out_file << "T";
                     break;
        }
    }
    
    out_file.close();
    return 0;
}
