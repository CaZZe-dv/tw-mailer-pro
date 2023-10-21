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

#define BUF 1024

using namespace std;

class FileManager{
    private:
        string mailboxPath;
        vector<string> readFile(const string& path, const int amount){
            vector<string> lines;
            ifstream inputFileStream(path);
            if(inputFileStream.is_open()){
                string helper;
                if(amount != -1){
                    for(int i = 0; i < amount; i++){
                        if(getline(inputFileStream,helper)){
                            lines.push_back(helper+'\n');
                        }
                    }
                }else{
                    while(getline(inputFileStream,helper)){
                        lines.push_back(helper+'\n');
                    }
                }
                inputFileStream.close();   
            }else{
                cerr << "Couldnt read file with given path: " << path << endl;
            }
            return lines;
        }

        bool writeFile(const string& path, vector<string> lines){
            ofstream outputFileStream(path);
            if(outputFileStream.is_open()){
                for(const string& line: lines){
                    outputFileStream << line << endl;
                }
                outputFileStream.close();
                return true;
            }else{
                cerr << "Couldnt write into file with given path: " << path << endl;
                return false;
            }
        }

        bool createDirectory(const string& path){
            if(filesystem::create_directory(path)){
                return true;
            }else{
                cerr << "User already has a inbox" << endl;
                return false;
            }
        }

        bool isDirectory(const string& path){
            return filesystem::is_directory(path);
        }

        bool createIndexFile(const string& path){
            vector<string> lines = {"1"};
            return writeFile(path,lines);
        }
        bool incrementIndexFile(const string& path){
            int index = stoi(readFile(path,1)[0]);
            vector<string> lines = {to_string(++index)};
            return writeFile(path,lines);
        }
        int getCurrentIndex(const string& path){
            return stoi(readFile(path,1)[0]);
        }
    public:
        FileManager(string mailboxPath){
            this-> mailboxPath = mailboxPath;
        }

        bool createMessage(string sender, string receiver, string subject, string message){
            string pathReceiver = mailboxPath+"/"+receiver;
            string pathIndex = pathReceiver+"/index.txt";
            if(createDirectory(pathReceiver)){
                createIndexFile(pathIndex);
            }
            int index = getCurrentIndex(pathIndex);
            string pathReceiverFile = pathReceiver+"/"+to_string(index)+".txt";
            vector<string> lines = {
                sender,
                receiver,
                subject,
                message
            };
            writeFile(pathReceiverFile,lines);
            incrementIndexFile(pathIndex);
            return true;
        }

        string listMessages(string username){
            string pathUsername = mailboxPath+"/"+username;
            string response, helper;
            int counter = 0;
            if(isDirectory(pathUsername)){
                for (const auto &entry : filesystem::directory_iterator(pathUsername)) {
                    if (filesystem::is_regular_file(entry)) {
                        if(entry.path().filename() == "index.txt"){
                            continue;
                        }
                        counter++;
                        helper.append(readFile(entry.path(),3)[2]);
                    }
                }
            }
            response.append(to_string(counter)+"\n");
            response.append(helper);
            return response;
        }  

        string readMessage(string username, int messageNumber){
            string pathUsername = mailboxPath+"/"+username;
            string response;
            int counter = 0;
            if(isDirectory(pathUsername)){
                for(const auto& entry : filesystem::directory_iterator(pathUsername)){
                    if (filesystem::is_regular_file(entry)) {
                        if(entry.path().filename() == "index.txt"){
                            continue;
                        }
                        counter++;
                        if(counter == messageNumber){
                            for(string line :readFile(entry.path(),-1)){
                                response.append(line);
                            }
                        }
                    }
                }
            }
            return response != "" ? "OK\n"+response : "ERR\n";
        }

        string delMessage(string username, int messageNumber){
            string pathUsername = mailboxPath+"/"+username;
            string response;
            int counter = 0;
            if(isDirectory(pathUsername)){
                for(const auto& entry : filesystem::directory_iterator(pathUsername)){
                    if (filesystem::is_regular_file(entry)) {
                        if(entry.path().filename() == "index.txt"){
                            continue;
                        }
                        counter++;
                        if(counter == messageNumber){
                            if(remove(entry.path())){
                                return "OK\n";
                            }
                        }
                    }
                }
            }
            return "ERR\n";
        }
};

string readLineMessage(istringstream& inputStream, int characters){
    string input;
    if(characters > 0){
        getline(inputStream,input);
        input = input.substr(0,characters);
    }else if(characters == -1){
        string helper;
        do{
            getline(inputStream,helper);
            if(helper != "."){
                input.append(helper+"\n");
            }
        }while(helper != ".");
    } 
    return input;  
}

string receiveDelProtocol(const string& message, FileManager& fileManager){
    istringstream inputStream(message);
    string protocol = readLineMessage(inputStream,4);
    string username = readLineMessage(inputStream,8);
    int messageNumber = stoi(readLineMessage(inputStream,10));
    return fileManager.delMessage(username,messageNumber);
}

string receiveReadProtocol(const string& message,FileManager& fileManager){
    istringstream inputStream(message);
    string protocol = readLineMessage(inputStream,4);
    string username = readLineMessage(inputStream,8);
    int messageNumber = stoi(readLineMessage(inputStream,10));
    return fileManager.readMessage(username,messageNumber);
}

string receiveListProtocol(const string& message,FileManager& fileManager){
    istringstream inputStream(message);
    string protocol = readLineMessage(inputStream,4);
    string username = readLineMessage(inputStream,8);
    return fileManager.listMessages(username);
}

string receiveSendProtocol(const string& message, FileManager& fileManager){
    istringstream inputStream(message);
    string protocol = readLineMessage(inputStream,10);
    string sender = readLineMessage(inputStream,8);
    string receiver = readLineMessage(inputStream,8);
    string subject = readLineMessage(inputStream,80);
    string text = readLineMessage(inputStream,-1);
    if(fileManager.createMessage(sender,receiver,subject,text)){
        return "OK\n";
    }else{
        return "ERR\n";
    }
}

bool sendMessageToClient(const int& clientSocket, const string& message){
    int sendStatus = send(clientSocket,message.c_str(),strlen(message.c_str()),0);
    if(sendStatus == -1){
        cerr << "Error occoured while sending message" << endl;
        return false;
    }
    return true;
}

string receiveMessageFromClient(const int& clientSocket, const int bufferSize){
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

int main(int argc, char *argv[]){
    const short PORT = atoi(argv[1]);
    const char* MAIL_SPOOL_DIRECTORYNAME = argv[2];
    
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

    FileManager fileManager(MAIL_SPOOL_DIRECTORYNAME);

    string message, response;
    do{
       message = receiveMessageFromClient(clientSocket,BUF);
       switch(message[0]){
            case 'S':
                response = receiveSendProtocol(message,fileManager);
                break;
            case 'L':
                response = receiveListProtocol(message,fileManager);
                break;
            case 'R':
                response = receiveReadProtocol(message,fileManager);
                break;
            case 'D':
                response = receiveDelProtocol(message,fileManager);
                break;
            case 'Q':
                cout << "Client disconnected from server" << endl;
                break;
            default:
                cerr << "Not a valid protocol given" << endl;
                break;
       }
        sendMessageToClient(clientSocket,response);
    }while(message[0] != 'Q');

    close(clientSocket);
    close(serverSocket);

    return EXIT_SUCCESS;
}