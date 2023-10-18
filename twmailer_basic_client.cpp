#include <iostream>
#include <cstring>
#include <string>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>
#include <utility>


#define BUF 1024

using namespace std;

class Protocol{
    private:
        vector<pair<string,int>> structure;
        string command;
    public:
        Protocol(string command, vector<pair<string,int>> &structure){
            this->command = command;
            this->structure = structure;
        }
        string fillProtocol(){
            string input, output;
            output.append(command+"\n");
            for(const auto& line : structure){
                cout << line.first << "(max " << line.second << " characters):" << endl;
                if(line.second != -1){
                    cin >> input;
                }else{
                    do{
                        cin >> input;
                        output.append(input+"\n");
                    }while(input != ".");
                    continue;
                }
                output.append(input+"\n");
            }
            return output;
        }
};

void printCommands(){
    cout << "Following commands are availible to communicate with Server: [SEND,LIST,READ,DEL,QUIT]" << endl;
    cout << "For more detailed explanation type ? infront of command (?SEND)" << endl;
}
//String wasnt used for comparison in switch because string cant be compared inside a switch, you would first have to transform it ito an integer
//which would cause unnecessary overhead, thats why we choose to just use the first letter of the command which is unique so it can be used to compare
//inside the switch
void printUsage(const char &command){
    switch(command){
        case 'S':
            cout << "SEND\\n\n<Sender>\\n\n<Receiver>\\n\n<Subject (max. 80 chars)>\\n\n<message (multi-line; no length restrictions)>\\n\n.\\n\n" << endl;
            break;
        case 'L':
            cout << "LIST\\n\n<Username>\\n\n" << endl;
            break;
        case 'R':
            cout << "READ\\n\n<Username>\\n\n<Message-Number>\\n\n" << endl;
            break;
        case 'D':
            cout << "DEL\\n\n<Username>\\n\n<Message-Number>\\n\n" << endl;
            break;
        case 'Q':
            cout << "QUIT\\n" << endl;
            break;
        default:
            cout << "No valid command has been given" << endl;
            break;   
    }
}

void sendMessageToServer(const int& clientSocket, const string& message){
    int sendStatus  = send(clientSocket,message.c_str(),strlen(message.c_str()),0);
    if(sendStatus == -1){
        cerr << "Error occoured while sending message" << endl;
    }
    cout << "Message has been sent to the server" << endl;
}

void send(const int& clientSocket){
    vector<pair<string,int>> structureSend = {
        {"Sender:",9},
        {"Receiver:",9},
        {"Subject:",80},
        {"message",-1}
    };
    Protocol p("SEND",structureSend);
    sendMessageToServer(clientSocket,p.fillProtocol());
}

void list(const int& clientSocket){
    vector<pair<string,int>> structureList = {
        {"Sender:",9}
    };
    Protocol p("LIST",structureList);
    sendMessageToServer(clientSocket,p.fillProtocol());
}

void read(const int& clientSocket){
    vector<pair<string,int>> structureRead = {
        {"Username:",9}
    };
    Protocol p("READ",structureRead);
    sendMessageToServer(clientSocket,p.fillProtocol());
}

void del(const int& clientSocket){
    vector<pair<string,int>> structureDel = {
        {"Username:",9},
        {"Message-Number:",10}
    };
    Protocol p("DEL",structureDel);
    sendMessageToServer(clientSocket,p.fillProtocol());
}

void quit(const int& clientSocket){
    vector<pair<string,int>> structureQuit;
    Protocol p("QUIT",structureQuit);
    sendMessageToServer(clientSocket,p.fillProtocol());
}



void chooseCommand(const char& command, const int& clientSocket){
    switch(command){
        case 'S':
            send(clientSocket);
            break;
        case 'L':
            list(clientSocket);
            break;
        case 'R':
            read(clientSocket);
            break;
        case 'D':
            del(clientSocket);
            break;
        case 'Q':
            quit(clientSocket);
            break;
        default:
            cout << "No valid command has been given" << endl;
            break;   
    }
}

int main(int argc, char* argv[]){
    const char* IP_ADDRESS = argv[1];
    const short PORT = atoi(argv[2]);

    int clientSocket = socket(AF_INET,SOCK_STREAM,0);

    if(clientSocket == -1){
        cerr << "Error occoured while creating clientsocket" << endl;
        return EXIT_FAILURE;
    }

    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET; // IPv4
    serverAddress.sin_port = htons(PORT); // Server port (in network byte order)
    inet_pton(AF_INET, IP_ADDRESS, &serverAddress.sin_addr);

    int connectionStatus = connect(clientSocket,(struct sockaddr*)&serverAddress,sizeof(serverAddress));

    if(connectionStatus == -1){
        cerr << "Error occoured while trying to connect to server" << endl;
        close(clientSocket);
        return EXIT_FAILURE;
    }

    string message;
    do{
        printCommands();

        cout << "Enter your command to be sent to the server: ";
        cin >> message;

        if(message[0] == '?'){
            printUsage(message[1]);
        }else{
            chooseCommand(message[0],clientSocket);
        }
    }while(message != "QUIT");

    close(clientSocket);

    return EXIT_SUCCESS;
}