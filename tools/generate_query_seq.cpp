// Generates all query sequences of a certain length of a given reference sequence

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <string>

using namespace std;

int main (int argc, char* argv[]) {
    if (argc < 4) {
        cout << "Usage: " << argv[0] << " <Ref Seq File> <Query Seq Length> <Output Filename>" << endl;
        exit(1);
    }

    ifstream ref_seq_file(argv[1]);
    char length_str[256];
    ref_seq_file.getline(length_str, 256);
    int ref_seq_length = atoi(length_str);
    
    ofstream out_file;
    
    string query = "";
    int query_length = atoi(argv[2]);
    out_file.open(argv[3]);
    out_file << ref_seq_length - query_length + 1 << endl;
    out_file << query_length << endl;
    for (int i = 0; i < ref_seq_length; i++) {
        query += ref_seq_file.get();
        if (query.length() > query_length) {
            query.erase(0, 1);
        }
        if (query.length() == query_length) {
            out_file << query << endl;
        }
    }
    
    out_file.close();
    return 0;
}
