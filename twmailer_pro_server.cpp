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
#include <filesystem>
#include <fstream>
#include <thread>
#include <unordered_map>
#include <functional>
#include <tuple>
#include "FileManager.hpp"
#include "UserVerificationLdap.hpp"
#include "Blacklist.hpp"
//Define buffer to be used when receiving data from socket
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 64
//Function to read specific amount of character of given string or -1 for multiline until . is reached
std::string readLineMessage(std::istringstream& inputStream, int characters) {
    std::string input;
    if (characters > 0) {
        getline(inputStream, input);
        input = input.substr(0, characters);
    }
    else if (characters == -1) {
        std::string helper;
        do {
            getline(inputStream, helper);
            if (helper != ".") {
                input.append(helper + "\n");
            }
        } while (helper != ".");
    }
    return input;
}
//Receiving the delete protocol the client has sent and preparing respone to client
std::string receiveDelProtocol(const std::string& message, TwmailerPro::FileManager& fileManager,const std::string& username) {
    std::istringstream inputStream(message);
    std::string protocol = readLineMessage(inputStream, 4);
    int messageNumber = std::stoi(readLineMessage(inputStream, 10));
    return fileManager.delMessage(username, messageNumber);
}
//Receiving read protocol of client and preparing response
std::string receiveReadProtocol(const std::string& message, TwmailerPro::FileManager& fileManager,const std::string& username) {
    std::istringstream inputStream(message);
    std::string protocol = readLineMessage(inputStream, 4);
    int messageNumber = std::stoi(readLineMessage(inputStream, 10));
    return fileManager.readMessage(username, messageNumber);
}
//Receiving list protocol of client and preparing response
std::string receiveListProtocol(const std::string& message, TwmailerPro::FileManager& fileManager,const std::string& username) {
    std::istringstream inputStream(message);
    std::string protocol = readLineMessage(inputStream, 4);
    return fileManager.listMessages(username);
}
//Receiving send protocol of cliet and preparing response
std::string receiveSendProtocol(const std::string& message, TwmailerPro::FileManager& fileManager,const std::string& sender) {
    std::istringstream inputStream(message);
    std::string protocol = readLineMessage(inputStream, 4);
    std::string receiver = readLineMessage(inputStream, 8);
    std::string subject = readLineMessage(inputStream, 80);
    std::string text = readLineMessage(inputStream, -1);
    std::cout << "BLUBBLAB" << std::endl;
    if (fileManager.createMessage(sender, receiver, subject, text)) {
        return "OK\n";
    }
    else {
        return "ERR\n";
    }
}

std::pair<std::string, std::string> receiveLoginProtocol(const std::string& message) {
    std::istringstream inputStream(message);
    std::string protocol = readLineMessage(inputStream, 5);
    std::string username = readLineMessage(inputStream, 20);
    std::string password = readLineMessage(inputStream, 20);
    return std::make_pair(username,password);
}
//Function to send message to client prints error when something went wrong also returns bool
bool sendMessageToClient(const int& clientSocket, const std::string& message) {
    int sendStatus = send(clientSocket, message.c_str(), strlen(message.c_str()), 0);
    if (sendStatus == -1) {
        std::cerr << "Error occurred while sending message" << std::endl;
        return false;
    }
    return true;
}
//Function to receive message of client return empty string when something went wrong and prints error message
std::string receiveMessageFromClient(const int& clientSocket, const int bufferSize) {
    char buffer[bufferSize];
    int bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (bytesRead == -1) {
        std::cerr << "Error occurred while receiving data from client" << std::endl;
        return "";
    } else {
        buffer[bytesRead] = '\0';
    }
    return buffer;
}

void handleClient(int clientSocket, TwmailerPro::FileManager& fileManager, TwmailerPro::UserVerificationLdap& uvl, TwmailerPro::Blacklist& blacklist){
    std::string message, response, protocol, username;
    bool loggedIn = false;
    do {
        message = receiveMessageFromClient(clientSocket, BUFFER_SIZE);
        std::istringstream inputStream(message);
        getline(inputStream,protocol);
        if(!loggedIn){
            if(protocol == "LOGIN"){
                std::pair<std::string, std::string> credentials = receiveLoginProtocol(message);
                response = uvl.bindLDAPCredentials(credentials.first.c_str(),credentials.second.c_str());
                if(response == "ERR\n"){
                    blacklist.isBlacklisted(credentials.first);
                }
                if(response == "OK\n"){
                    if(blacklist.isBlacklisted(credentials.first)){
                        response == "ERR\n";
                    }else{
                        username = credentials.first;
                        loggedIn = true;
                    }
                }
            }else if(protocol != "QUIT"){
                std::cout << "Invalid command has been sent no action performed" << std::endl;
            }
        }else{
            if(protocol == "SEND"){
                response = receiveSendProtocol(message,fileManager,username);
            }else if(protocol == "LIST"){
                response = receiveListProtocol(message,fileManager,username);
            }else if(protocol == "READ"){
                response = receiveReadProtocol(message,fileManager,username);
            }else if(protocol == "DEL"){
                response = receiveDelProtocol(message,fileManager,username);
            }else if(protocol != "QUIT"){
                std::cout << "Invalid command has been sent no action performed" << std::endl; 
            }   
        }
        sendMessageToClient(clientSocket, response);
    } while (protocol != "QUIT");
    close(clientSocket);
}

bool establishConnection(int& serverSocket, const unsigned short& port){
    // AF_INET for IPv4 protocol
    // SOCK_STREAM sets the type of communication in this case TCP
    // Last parameter set to 0 to be set by the operating system
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    // If an error occurs while creating the socket object, the method returns -1
    // So we print an error and exit the program with exit code 1 == failure
    if (serverSocket == -1) {
        std::cerr << "Error occurred while creating a socket for the server" << std::endl;
        return false;
    }
    // Define sockaddr_in struct to declare the IP address, port, and communication protocol
    struct sockaddr_in serverAddress;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_family = AF_INET;
    // htons is used to transform the port number to network byte order
    serverAddress.sin_port = htons(port);

    int bindStatus = bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
    if (bindStatus == -1) {
        std::cerr << "Error occurred while binding IP and port to the server socket" << std::endl;
        close(serverSocket);
        return false;
    }
    // After IP and port have been successfully bound to the socket, we can start listening for clients
    // therefore, we give our socket and flag to how many clients the server should listen
    // because specifications for a basic model is only one client, backlog flag is set to 1
    // allowing only one client to be accepted by the server
    int listenStatus = listen(serverSocket, MAX_CLIENTS);
    // If the flag returns -1, an error occurred while listening for clients, therefore, an error is printed
    // and the server socket is closed, and the program is exited with an error
    if (listenStatus == -1) {
        std::cerr << "Error occurred while listening for connection" << std::endl;
        close(serverSocket);
        return false;
    }
    return true;
}

int main(int argc, char* argv[]) {
    const unsigned short PORT = atoi(argv[1]);
    const char* MAIL_SPOOL_DIRECTORYNAME = argv[2];

    int serverSocket;

    if(!establishConnection(serverSocket,PORT)){
        return EXIT_FAILURE;
    }
    std::cout << "Server is listening to port: " << PORT << std::endl;

    TwmailerPro::FileManager fileManager(MAIL_SPOOL_DIRECTORYNAME);
    TwmailerPro::UserVerificationLdap uvl;
    TwmailerPro::Blacklist blacklist;

    struct sockaddr_in clientAddress;
    socklen_t clientAddressLength = sizeof(clientAddress);

    while (true) {
        std::cout << "Accepting clients" << std::endl;
        int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddress, &clientAddressLength);
        std::cout << clientSocket << std::endl;
        if (clientSocket == -1) {
            std::cerr << "Error occurred while accepting a client" << std::endl;
            close(clientSocket);
            continue;
        }
        std::thread thread(std::bind(handleClient, clientSocket, std::ref(fileManager), std::ref(uvl), std::ref(blacklist)));
        thread.detach();  // Detach the thread, so it can run independently
    }

    close(serverSocket);

    return EXIT_SUCCESS;
}