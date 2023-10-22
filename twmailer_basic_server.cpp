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
//Define buffer to be used when receiving data from socket
const int BUFFER_SIZE = 1024;
//Class for managing everything with files and also creating responses to be sent to client
class FileManager {
private:
    //Path for the mailspool directory
    std::string mailboxPath;
    //Method to read specific amount of lines to a given file as path or -1 to read the whole file
    std::vector<std::string> readFile(const std::string& path, const int amount) {
        std::vector<std::string> lines;
        std::ifstream inputFileStream(path);
        if (inputFileStream.is_open()) {
            std::string helper;
            if (amount != -1) {
                for (int i = 0; i < amount; i++) {
                    if (getline(inputFileStream, helper)) {
                        lines.push_back(helper + '\n');
                    }
                }
            }
            else {
                while (getline(inputFileStream, helper)) {
                    lines.push_back(helper + '\n');
                }
            }
            inputFileStream.close();
        }
        else {
            std::cerr << "Could not read file with the given path: " << path << std::endl;
        }
        return lines;
    }
    //Method to write into file given as path with specified lines in for mof vector
    //doesnt append lines overrides when theres already something in file
    bool writeFile(const std::string& path, std::vector<std::string> lines) {
        std::ofstream outputFileStream(path);
        if (outputFileStream.is_open()) {
            for (const std::string& line : lines) {
                outputFileStream << line << std::endl;
            }
            outputFileStream.close();
            return true;
        }
        else {
            std::cerr << "Could not write into the file with the given path: " << path << std::endl;
            return false;
        }
    }
    //Methode to create a directory by given path, returns true when directory successfully created, false when already exists
    bool createDirectory(const std::string& path) {
        if (std::filesystem::create_directory(path)) {
            return true;
        }
        else {
            std::cerr << "User already has an inbox" << std::endl;
            return false;
        }
    }
    //Method to check if given path is directory
    bool isDirectory(const std::string& path) {
        return std::filesystem::is_directory(path);
    }
    //Creating an index file to given path, to track number of next message to be created
    bool createIndexFile(const std::string& path) {
        std::vector<std::string> lines = { "1" };
        return writeFile(path, lines);
    }
    //Method to increment the number in index file
    bool incrementIndexFile(const std::string& path) {
        int index = std::stoi(readFile(path, 1)[0]);
        std::vector<std::string> lines = { std::to_string(++index) };
        return writeFile(path, lines);
    }
    //Fetches the current index of index file 
    int getCurrentIndex(const std::string& path) {
        return std::stoi(readFile(path, 1)[0]);
    }
public:
    FileManager(std::string mailboxPath) {
        this->mailboxPath = mailboxPath;
    }
    //Creates file in receiver directory with all contents passed to function
    bool createMessage(std::string sender, std::string receiver, std::string subject, std::string message) {
        std::string pathReceiver = mailboxPath + "/" + receiver;
        std::string pathIndex = pathReceiver + "/index.txt";
        if (createDirectory(pathReceiver)) {
            createIndexFile(pathIndex);
        }
        int index = getCurrentIndex(pathIndex);
        std::string pathReceiverFile = pathReceiver + "/" + std::to_string(index) + ".txt";
        std::vector<std::string> lines = {
            sender,
            receiver,
            subject,
            message
        };
        writeFile(pathReceiverFile, lines);
        incrementIndexFile(pathIndex);
        return true;
    }
    //Lists all message subjects of given user
    std::string listMessages(std::string username) {
        std::string pathUsername = mailboxPath + "/" + username;
        std::string response, helper;
        int counter = 0;
        if (isDirectory(pathUsername)) {
            for (const auto& entry : std::filesystem::directory_iterator(pathUsername)) {
                if (std::filesystem::is_regular_file(entry)) {
                    if (entry.path().filename() == "index.txt") {
                        continue;
                    }
                    counter++;
                    helper.append(readFile(entry.path(), 3)[2]);
                }
            }
        }
        response.append(std::to_string(counter) + "\n");
        response.append(helper);
        return response;
    }
    //Reads the full content of message from given user and message number
    std::string readMessage(std::string username, int messageNumber) {
        std::string pathUsername = mailboxPath + "/" + username;
        std::string response;
        int counter = 0;
        if (isDirectory(pathUsername)) {
            for (const auto& entry : std::filesystem::directory_iterator(pathUsername)) {
                if (std::filesystem::is_regular_file(entry)) {
                    if (entry.path().filename() == "index.txt") {
                        continue;
                    }
                    counter++;
                    if (counter == messageNumber) {
                        for (std::string line : readFile(entry.path(), -1)) {
                            response.append(line);
                        }
                    }
                }
            }
        }
        return response != "" ? "OK\n" + response : "ERR\n";
    }
    //Deletes a specific message of directory of given user and number
    std::string delMessage(std::string username, int messageNumber) {
        std::string pathUsername = mailboxPath + "/" + username;
        std::string response;
        int counter = 0;
        if (isDirectory(pathUsername)) {
            for (const auto& entry : std::filesystem::directory_iterator(pathUsername)) {
                if (std::filesystem::is_regular_file(entry)) {
                    if (entry.path().filename() == "index.txt") {
                        continue;
                    }
                    counter++;
                    if (counter == messageNumber) {
                        if (std::filesystem::remove(entry.path())) {
                            return "OK\n";
                        }
                    }
                }
            }
        }
        return "ERR\n";
    }
};
//Fuction to read specific amount of character of given string or -1 for multiline until . is reached
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
std::string receiveDelProtocol(const std::string& message, FileManager& fileManager) {
    std::istringstream inputStream(message);
    std::string protocol = readLineMessage(inputStream, 4);
    std::string username = readLineMessage(inputStream, 8);
    int messageNumber = std::stoi(readLineMessage(inputStream, 10));
    return fileManager.delMessage(username, messageNumber);
}
//Receiving read protocol of client and preparing response
std::string receiveReadProtocol(const std::string& message, FileManager& fileManager) {
    std::istringstream inputStream(message);
    std::string protocol = readLineMessage(inputStream, 4);
    std::string username = readLineMessage(inputStream, 8);
    int messageNumber = std::stoi(readLineMessage(inputStream, 10));
    return fileManager.readMessage(username, messageNumber);
}
//Receiving list protocol of client and preparing response
std::string receiveListProtocol(const std::string& message, FileManager& fileManager) {
    std::istringstream inputStream(message);
    std::string protocol = readLineMessage(inputStream, 4);
    std::string username = readLineMessage(inputStream, 8);
    return fileManager.listMessages(username);
}
//Receiving send protocol of cliet and preparing response
std::string receiveSendProtocol(const std::string& message, FileManager& fileManager) {
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

    FileManager fileManager(MAIL_SPOOL_DIRECTORYNAME);

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