#include <signal.h>
#include <iostream>
#include <fstream>
#include <cmath>
#include <cstring>

using namespace std;

void my_handler(int s){
    cout << "\e[?25h";
    exit(1);
}

void nums_loop(string filename, int part, int total){
    cout << "\e[?25l";
    signal (SIGINT, my_handler);

    ofstream file;
    file.open (filename);
    for(part; part < total+1; part++){
        file << part << endl;
        cout << "\rloading: [ " << total << "/" << part << " ]";
    }
    file.close();
    cout << "\e[?25h";
}

void strings_loop(string filename, string data){

    if (data.length() > 9){
        cout << "# max length is 9 chars !" << endl;
        exit(1);
    } else if (data.length() < 2){
        cout << "# min length is 2 chars !" << endl;
        exit(1);
    }

    char array_data[data.length()][data.length()];
    int total = pow(data.length(),data.length());
    int part = 0;

    // add the string data total an array.
    for (int i=0; i < data.length(); i++){
        for(int j=0; j < data.length(); j++){
            array_data[i][j] = data[j];
        }
    }

    cout << "\e[?25l";
    signal (SIGINT, my_handler);

    ofstream file;
    file.open (filename);

    if (data.length() == 2){
        for(int a=0; a < data.length(); a++){
            for(int b=0; b < data.length(); b++){
                file << array_data[0][a] << array_data[1][b] << endl;
                part++;
                cout << "\rloading: [ " << total << "/" << part << " ]";
            }
        }
    } else if (data.length() == 3){
        for(int a=0; a < data.length(); a++){
            for(int b=0; b < data.length(); b++){
                for(int c=0; c < data.length(); c++){
                    file << array_data[0][a] << array_data[1][b] << array_data[2][c] << endl;
                    part++;
                    cout << "\rloading: [ " << total << "/" << part << " ]";
                }
            }
        }
    } else if (data.length() == 4){
        for(int a=0; a < data.length(); a++){
            for(int b=0; b < data.length(); b++){
                for(int c=0; c < data.length(); c++){
                    for(int d=0; d < data.length(); d++){
                        file << array_data[0][a] << array_data[1][b] << array_data[2][c] << array_data[2][d] << endl;
                        part++;
                        cout << "\rloading: [ " << total << "/" << part << " ]";
                    }
                }
            }
        }
    } else if (data.length() == 5){
        for(int a=0; a < data.length(); a++){
            for(int b=0; b < data.length(); b++){
                for(int c=0; c < data.length(); c++){
                    for(int d=0; d < data.length(); d++){
                        for(int e=0; e < data.length(); e++){
                            file << array_data[0][a] << array_data[1][b] << array_data[2][c] << array_data[2][d] << array_data[2][e] << endl;
                            part++;
                            cout << "\rloading: [ " << total << "/" << part << " ]";
                        }
                    }
                }
            }
        }
    } else if (data.length() == 6){
        for(int a=0; a < data.length(); a++){
            for(int b=0; b < data.length(); b++){
                for(int c=0; c < data.length(); c++){
                    for(int d=0; d < data.length(); d++){
                        for(int e=0; e < data.length(); e++){
                            for(int f=0; f < data.length(); f++){
                                file << array_data[0][a] << array_data[1][b] << array_data[2][c] << array_data[2][d] << array_data[2][e] << array_data[2][f] << endl;
                                part++;
                                cout << "\rloading: [ " << total << "/" << part << " ]";
                            }
                        }
                    }
                }
            }
        }
    } else if (data.length() == 7){
        for(int a=0; a < data.length(); a++){
            for(int b=0; b < data.length(); b++){
                for(int c=0; c < data.length(); c++){
                    for(int d=0; d < data.length(); d++){
                        for(int e=0; e < data.length(); e++){
                            for(int f=0; f < data.length(); f++){
                                for(int g=0; g < data.length(); g++){
                                    file << array_data[0][a] << array_data[1][b] << array_data[2][c] << array_data[2][d] << array_data[2][e] << array_data[2][f] << array_data[2][g] << endl;
                                    part++;
                                    cout << "\rloading: [ " << total << "/" << part << " ]";
                                }
                            }
                        }
                    }
                }
            }
        }
    } else if (data.length() == 8){
        for(int a=0; a < data.length(); a++){
            for(int b=0; b < data.length(); b++){
                for(int c=0; c < data.length(); c++){
                    for(int d=0; d < data.length(); d++){
                        for(int e=0; e < data.length(); e++){
                            for(int f=0; f < data.length(); f++){
                                for(int g=0; g < data.length(); g++){
                                    for(int h=0; h < data.length(); h++){
                                        file << array_data[0][a] << array_data[1][b] << array_data[2][c] << array_data[2][d] << array_data[2][e] << array_data[2][f] << array_data[2][g] << array_data[2][h] << endl;
                                        part++;
                                        cout << "\rloading: [ " << total << "/" << part << " ]";
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    } else if (data.length() == 9){
        for(int a=0; a < data.length(); a++){
            for(int b=0; b < data.length(); b++){
                for(int c=0; c < data.length(); c++){
                    for(int d=0; d < data.length(); d++){
                        for(int e=0; e < data.length(); e++){
                            for(int f=0; f < data.length(); f++){
                                for(int g=0; g < data.length(); g++){
                                    for(int h=0; h < data.length(); h++){
                                        for(int k=0; k < data.length(); k++){
                                            file << array_data[0][a] << array_data[1][b] << array_data[2][c] << array_data[2][d] << array_data[2][e] << array_data[2][f] << array_data[2][g] << array_data[2][h] << array_data[2][k] << endl;
                                            part++;
                                            cout << "\rloading: [ " << total << "/" << part << " ]";
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    file.close();
    cout << "\e[?25h";

}

void help_message(){
    system("readme plist");
    exit(1);
}

int main(int argc, char* argv[]){
    string filename = "";
    string type = "";
    for(int i=0;i < argc; i++){
        if (string(argv[i]) == "-o"){
            if (i+1 < argc){
                filename = string(argv[i+1]);
            } else {
                cout << "# file not found!" << endl;
                help_message();
            }
        } else if (string(argv[i]) == "-t"){
            if (i+1 < argc){
                if (string(argv[i+1]) == "nums" || string(argv[i+1]) == "strings"){
                    type = string(argv[i+1]);
                } else{
                    cout << "# Error:" + string(argv[i+1]) + ": select [nums or strings]"  << endl;
                    help_message();
                }
            } else {
                cout << "# no type selected!" << endl;
                help_message();
            }
        }
    }

    if(filename != ""  && type != ""){
        if (type == "nums"){
            nums_loop(filename, stoi(string(argv[argc-2])), stoi(string(argv[argc-1])));
        } else if(type == "strings") {
            strings_loop(filename, string(argv[argc-1]));
        } else {
            help_message();
        }
    } else {
        help_message();
    }
    return 0;
}
