#include <iostream>
#include <cstring>
#include <string>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>
#include <utility>
#include <map>
#include <cstdlib>

#define BUF 1024

using namespace std;

class Protocol{
    private:
        vector<pair<string,int>> structure;
        string command;
    public:
        Protocol() {
            
        }   
        Protocol(string command){
            this->command = command;
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
        void addLineToStructure(const string& s, const int i){
            structure.push_back(pair<string,int>(s,i));
        }
};

void sendMessageToServer(const int& clientSocket, const string& message){
    int sendStatus  = send(clientSocket,message.c_str(),strlen(message.c_str()),0);
    if(sendStatus == -1){
        cerr << "Error occoured while sending message" << endl;
    }
    cout << "Message has been sent to the server" << endl;
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

    map<char,Protocol> protocols;
    //Send protocol
    Protocol send("SEND");
    send.addLineToStructure("Sender",9);
    send.addLineToStructure("Receiver",9);
    send.addLineToStructure("Subject",80);
    send.addLineToStructure("Message",-1);
    //List protocol
    Protocol list("LIST");
    list.addLineToStructure("Username",9);
    //Read protocol
    Protocol read("READ");
    read.addLineToStructure("Username",9);
    read.addLineToStructure("Message-Number",10);
    //Del protocol
    Protocol del("DEL");
    del.addLineToStructure("Username",9);
    del.addLineToStructure("Message-Number",10);
    //Quit Protocol
    Protocol quit("QUIT");
    //Associate protocols with user input S => send protocol is called
    //protocols.insert(pair<char,Protocol>('S',send));
    //protocols.insert(pair<char,Protocol>('L',list));
    //protocols.insert(pair<char,Protocol>('R',read));
    //protocols.insert(pair<char,Protocol>('D',del));
    //protocols.insert(pair<char,Protocol>('Q',quit));
    protocols['S'] = send;
    protocols['L'] = list;
    protocols['R'] = read;
    protocols['D'] = del;
    protocols['Q'] = quit;

    string message;
    do{
        cout << "Enter your command to be sent to the server [SEND,LIST,READ,DEL,QUIT]: ";
        cin >> message;
        string protocolMessage = protocols[message[0]].fillProtocol();
        sendMessageToServer(clientSocket,protocolMessage);
        char buffer[BUF];
        int bytesRead = recv(clientSocket,buffer,sizeof(buffer),0);

        if(bytesRead == -1){
            cerr << "Error occoured while receiving data from client" << endl;
        }else{
            buffer[bytesRead] = '\0';
            cout << buffer << endl;
        }
    }while(message != "QUIT");

    close(clientSocket);

    return EXIT_SUCCESS;
}