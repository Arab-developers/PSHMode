#include <signal.h>
#include <iostream>
#include <fstream>
#include <cmath>

using namespace std;

ofstream file;

void argvHandler(int, char* [], string &, string &);
void errorHandler(int);
void numsLoop(string, int, int);
void stringsLoop(string, string, string chars = "", int index = -1);
void helpMessage();

int main(int argc, char* argv[]){
    string filename = "";
    string type = "";

    argvHandler(argc, argv, filename, type);

    if(filename != ""  && type != ""){
        if (type == "nums"){
            numsLoop(filename, stoi(string(argv[argc-2])), stoi(string(argv[argc-1])));
        } else if(type == "strings") {
            stringsLoop(filename, string(argv[argc-1]));
        } else {
            helpMessage();
        }
    } else {
        helpMessage();
    }
    return 0;
}

void argvHandler(int argc, char* argv[], string &filename, string &type){
    for(int i=0;i < argc; i++){
        if (string(argv[i]) == "-o"){
            if (i+1 < argc){
                filename = string(argv[i+1]);
            } else {
                cout << "# file not found!" << endl;
                helpMessage();
            }
        } else if (string(argv[i]) == "-t"){
            if (i+1 < argc){
                if (string(argv[i+1]) == "nums" || string(argv[i+1]) == "strings"){
                    type = string(argv[i+1]);
                } else {
                    cout << "# Error:" + string(argv[i+1]) + ": select [nums or strings]"  << endl;
                    helpMessage();
                }
            } else {
                cout << "# no type selected!" << endl;
                helpMessage();
            }
        }
    }
}

void errorHandler(int s){
    cout << "\e[?25h";
    exit(1);
}

void numsLoop(string filename, int part, int total){
    cout << "\e[?25l";
    signal (SIGINT, errorHandler);
    file.open (filename);
    for(part; part < total+1; part++){
        file << part << endl;
        cout << "\rloading: [ " << total << "/" << part << " ]";
    }
    file.close();
    cout << "\e[?25h";
}

void stringsLoop(string filename, string data, string chars, int index){
    static int total = pow(data.length(), data.length());
    static int part = 0;
    cout << "\e[?25l";
    signal (SIGINT, errorHandler);
    if (index == -1) {
        file.open (filename);
        index = data.length();
    }
    if (index == 0){
        if (part+1 == total) file << chars; else file << chars << endl;
        part++;
        cout << "\rloading: [ " << total << "/" << part << " ]";
    } else {
        for (int i=0; i < data.length(); i++)
            stringsLoop(filename, data, chars + data[i], index-1);
    }
    if ( total == part ){
        file.close();
        cout << "\e[?25h";
    }
}

void helpMessage(){
    system("readme plist");
    exit(1);
}