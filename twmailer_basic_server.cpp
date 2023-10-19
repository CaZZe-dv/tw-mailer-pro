#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <iostream>
#include <vector>
#include <utility>
#include <map>
#include <sstream>

#define PORT 5555
#define BUF 1024

using namespace std;

class ProtocolReceive{
    private:
        vector<pair<string,int>> structure;
        map<string,string> variables;
    public:
        void fillProtocol(string& message){
            istringstream inputStream(message);
            string output;
            getline(inputStream,output,'\n');
            for(const auto& line : structure){
                if(line.second != -1){
                    getline(inputStream,output,'\n');
                }else{
                    string helper;
                    while(getline(inputStream,helper,'\n')){
                        if(helper != "."){
                            output += helper+"\n";
                        }
                    }
                }
                addVariables(line.first,output);
            }
        }
        void addLineToStructure(const string& s, const int i){
            structure.push_back(pair<string,int>(s,i));
        }
        void addVariables(const string& key, const string& value){
            variables[key] = value;
        }
};

void sendMessageToClient(const int& clientSocket, const string& message){
    int sendStatus  = send(clientSocket,message.c_str(),strlen(message.c_str()),0);
    if(sendStatus == -1){
        cerr << "Error occoured while sending message" << endl;
    }
    cout << "Message has been sent to the client" << endl;
}

int main(int argc, char *argv[]){
    //AF_INET for IPv4 protocol
    //SOCK_STREAM sets the type of communication in this case TCP
    //Last parameter set to 0 to be set by operating system
    int serverSocket = socket(AF_INET,SOCK_STREAM,0);
    //If error occours while creating socket object method returns -1
    //So we print error and exit the program with exit code 1 == failure
    if(serverSocket == -1){
        cerr << "Error occoured while creating socket for server" << endl;
        return EXIT_FAILURE;
    }
    //Define sockaddr_in struct to declare ip adress, port and communication protocol
    struct sockaddr_in serverAddress;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_family = AF_INET;
    //htons is used to transform port number to network byte order
    serverAddress.sin_port = htons(PORT);

    int bindStatus = bind(serverSocket,(struct sockaddr*)&serverAddress,sizeof(serverAddress));
    if(bindStatus == -1){
        cerr << "Error occoured while binding ip and port to serversocket" << endl;
        close(serverSocket);
        return EXIT_FAILURE;
    }
    //After IP and port has been successfully bond to socket, we can start listening for clients
    //therefore we give our socket and flag to how many clients server should listen
    //because specifications for basic model is only one client backlog flag is set to 1
    //allowing only one client to be accepted by the server
    int listenStatus = listen(serverSocket, 1);
    //If flag returns -1 error occoured while listening for clients therefore, error is printed
    //and seversocket is closed and program exited with error
    if(listenStatus == -1){
        cerr << "Error occoured while listening for connection" << endl;
        close(serverSocket);
        return EXIT_FAILURE;
    }
    cout << "Server is listening to port: " << PORT << endl;

    struct sockaddr_in clientAddress;
    socklen_t clientAddressLength = sizeof(clientAddress);
    int clientSocket = accept(serverSocket,(struct sockaddr*)&clientAddress,&clientAddressLength);

    if(clientSocket == -1){
        cerr << "Error ocoured while accepting client" << endl;
        close(serverSocket);
        return EXIT_FAILURE;
    }
    map<char,ProtocolReceive> protocols;
    ProtocolReceive receiveSend;
    receiveSend.addLineToStructure("Sender",9);
    receiveSend.addLineToStructure("Receiver",9);
    receiveSend.addLineToStructure("Subject",80);
    receiveSend.addLineToStructure("Message",-1);
    ProtocolReceive receiveList;
    receiveSend.addLineToStructure("Username",9);
    ProtocolReceive receiveRead;
    receiveSend.addLineToStructure("Username",9);
    receiveSend.addLineToStructure("Message-Number",10);
    ProtocolReceive receiveDel;
    receiveSend.addLineToStructure("Username",9);
    receiveSend.addLineToStructure("Message-Number",10);
    ProtocolReceive receiveQuit;

    protocols['S'] = receiveSend;
    protocols['L'] = receiveList;
    protocols['R'] = receiveRead;
    protocols['D'] = receiveDel;
    protocols['Q'] = receiveQuit;

    
    string message;

    do{
        char buffer[BUF];
        int bytesRead = recv(clientSocket,buffer,sizeof(buffer),0);
        if(bytesRead == -1){
            cerr << "Error occoured while receiving data from client" << endl;
        }else{
            buffer[bytesRead] = '\0';
            istringstream inputStream(message);
            getline(inputStream,message,'\n');
            protocols[message[0]].fillProtocol(message);
        }

        sendMessageToClient(clientSocket,"OK big chief");
    }while(message != "QUIT");

    close(clientSocket);
    close(serverSocket);

    return EXIT_SUCCESS;
}