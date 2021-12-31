#include <string>
#include <bitset>
#include <iostream>
#include <sstream>

using namespace std;

void encodeBinary(string);
void decodeBinary(string);


int main(int argc, char *argvs[]){
    if (argc >= 3){
        if (string(argvs[1]) == "-d")
            decodeBinary(argvs[2]);
        else if (string(argvs[1]) == "-e")
            encodeBinary(argvs[2]);
        else
            system("readme binary");
    } else {
        cout << argc << '\n';
        system("readme binary");
    } 
    cout << '\n';
    return 0;
}


void encodeBinary(string data){
    for (std::size_t i = 0; i < data.size(); ++i)
        cout << bitset<8>(data.c_str()[i]) << ' ';
}

void decodeBinary(string data){
    stringstream sstream(data);
    while(sstream.good()) {
        bitset<8> bits;
        sstream >> bits;
        cout << char(bits.to_ulong());
    }
}
