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

string readFromConsole(string hint, int characters){
    string input;
    if(characters > 0){
        cout << hint << "(max. " << characters << "characters): ";
        getline(cin,input);
    }else if(characters == -1){
        cout << hint << "(no limit, multiline):" << endl;
        string helper;
        do{
            getline(cin,helper);
            input.append(helper+"\n");
        }while(helper != ".");
    } 
    return input+'\n';  
}

string createQuitProtocol(){
    string message;
    message.append("QUIT\n");
    return message;
}

string createDelProtocol(){
    string message;
    message.append("DEL\n");
    message.append(readFromConsole("Username: ",8));
    message.append(readFromConsole("Message-Number: ",10));
    return message;
}

string createReadProtocol(){
    string message;
    message.append("READ\n");
    message.append(readFromConsole("Username: ",8));
    message.append(readFromConsole("Message-Number: ",10));
    return message;
}

string createListProtocol(){
    string message;
    message.append("LIST\n");
    message.append(readFromConsole("Username: ",8));
    return message;
}

string createSendProtocol(){
    string message;
    message.append("SEND\n");
    message.append(readFromConsole("Sender: ",8));
    message.append(readFromConsole("Receiver: ",8));
    message.append(readFromConsole("Subject: ",80));
    message.append(readFromConsole("Message: ",-1));
    return message;
}

bool sendMessageToServer(const int& clientSocket, const string& message){
    int sendStatus = send(clientSocket,message.c_str(),strlen(message.c_str()),0);
    if(sendStatus == -1){
        cerr << "Error occoured while sending message" << endl;
        return false;
    }
    return true;
}

string receiveMessageFromServer(const int& clientSocket, const int bufferSize){
    char buffer[bufferSize];
    int bytesRead = recv(clientSocket,buffer,sizeof(buffer),0);
    if(bytesRead == -1){
        cerr << "Error occoured while receiving data from client" << endl;
        return "";
    }else{
        buffer[bytesRead] = '\0';
    }
    return buffer;
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

    string userInput;
    string protocol;
    do{
        cout << "Enter your command to be sent to the server [SEND,LIST,READ,DEL,QUIT]: ";
        getline(cin,userInput);
        switch(userInput[0]){
            case 'S':
                protocol = createSendProtocol();
                break;
            case 'L':
                protocol = createListProtocol();
                break;
            case 'R':
                protocol = createReadProtocol();
                break;
            case 'D':
                protocol = createDelProtocol();
                break;
            case 'Q':
                protocol = createQuitProtocol();
                break;
            default:
                cerr << "Not a valid command given" << endl;
                break;    
        }

        if(!sendMessageToServer(clientSocket,protocol)){
            cerr << "Couldnt transmit protocol to server" << endl;
        }

        cout << receiveMessageFromServer(clientSocket,BUF) << endl;
    }while(userInput[0] != 'Q');

    close(clientSocket);

    return EXIT_SUCCESS;
}