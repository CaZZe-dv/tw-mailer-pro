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
#include "FileManager.hpp"
//Define buffer to be used when receiving data from socket
const int BUFFER_SIZE = 1024;
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
std::string receiveDelProtocol(const std::string& message, TwmailerPro::FileManager& fileManager) {
    std::istringstream inputStream(message);
    std::string protocol = readLineMessage(inputStream, 4);
    std::string username = readLineMessage(inputStream, 8);
    int messageNumber = std::stoi(readLineMessage(inputStream, 10));
    return fileManager.delMessage(username, messageNumber);
}
//Receiving read protocol of client and preparing response
std::string receiveReadProtocol(const std::string& message, TwmailerPro::FileManager& fileManager) {
    std::istringstream inputStream(message);
    std::string protocol = readLineMessage(inputStream, 4);
    std::string username = readLineMessage(inputStream, 8);
    int messageNumber = std::stoi(readLineMessage(inputStream, 10));
    return fileManager.readMessage(username, messageNumber);
}
//Receiving list protocol of client and preparing response
std::string receiveListProtocol(const std::string& message, TwmailerPro::FileManager& fileManager) {
    std::istringstream inputStream(message);
    std::string protocol = readLineMessage(inputStream, 4);
    std::string username = readLineMessage(inputStream, 8);
    return fileManager.listMessages(username);
}
//Receiving send protocol of cliet and preparing response
std::string receiveSendProtocol(const std::string& message, TwmailerPro::FileManager& fileManager) {
    std::istringstream inputStream(message);
    std::string protocol = readLineMessage(inputStream, 10);
    std::string sender = readLineMessage(inputStream, 8);
    std::string receiver = readLineMessage(inputStream, 8);
    std::string subject = readLineMessage(inputStream, 80);
    std::string text = readLineMessage(inputStream, -1);
    if (fileManager.createMessage(sender, receiver, subject, text)) {
        return "OK\n";
    }
    else {
        return "ERR\n";
    }
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

int main(int argc, char* argv[]) {
    const short PORT = atoi(argv[1]);
    const char* MAIL_SPOOL_DIRECTORYNAME = argv[2];

    // AF_INET for IPv4 protocol
    // SOCK_STREAM sets the type of communication in this case TCP
    // Last parameter set to 0 to be set by the operating system
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    // If an error occurs while creating the socket object, the method returns -1
    // So we print an error and exit the program with exit code 1 == failure
    if (serverSocket == -1) {
        std::cerr << "Error occurred while creating a socket for the server" << std::endl;
        return EXIT_FAILURE;
    }
    // Define sockaddr_in struct to declare the IP address, port, and communication protocol
    struct sockaddr_in serverAddress;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_family = AF_INET;
    // htons is used to transform the port number to network byte order
    serverAddress.sin_port = htons(PORT);

    int bindStatus = bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
    if (bindStatus == -1) {
        std::cerr << "Error occurred while binding IP and port to the server socket" << std::endl;
        close(serverSocket);
        return EXIT_FAILURE;
    }
    // After IP and port have been successfully bound to the socket, we can start listening for clients
    // therefore, we give our socket and flag to how many clients the server should listen
    // because specifications for a basic model is only one client, backlog flag is set to 1
    // allowing only one client to be accepted by the server
    int listenStatus = listen(serverSocket, 1);
    // If the flag returns -1, an error occurred while listening for clients, therefore, an error is printed
    // and the server socket is closed, and the program is exited with an error
    if (listenStatus == -1) {
        std::cerr << "Error occurred while listening for connection" << std::endl;
        close(serverSocket);
        return EXIT_FAILURE;
    }
    std::cout << "Server is listening to port: " << PORT << std::endl;

    struct sockaddr_in clientAddress;
    socklen_t clientAddressLength = sizeof(clientAddress);
    int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddress, &clientAddressLength);

    if (clientSocket == -1) {
        std::cerr << "Error occurred while accepting a client" << std::endl;
        close(serverSocket);
        return EXIT_FAILURE;
    }

    TwmailerPro::FileManager fileManager(MAIL_SPOOL_DIRECTORYNAME);

    std::string message, response;
    do {
        message = receiveMessageFromClient(clientSocket, BUFFER_SIZE);
        switch (message[0]) {
        case 'S':
            response = receiveSendProtocol(message, fileManager);
            break;
        case 'L':
            response = receiveListProtocol(message, fileManager);
            break;
        case 'R':
            response = receiveReadProtocol(message, fileManager);
            break;
        case 'D':
            response = receiveDelProtocol(message, fileManager);
            break;
        case 'Q':
            std::cout << "Client disconnected from the server" << std::endl;
            break;
        default:
            std::cerr << "Not a valid protocol given" << std::endl;
            break;
        }
        sendMessageToClient(clientSocket, response);
    } while (message[0] != 'Q');

    close(clientSocket);
    close(serverSocket);

    return EXIT_SUCCESS;
}