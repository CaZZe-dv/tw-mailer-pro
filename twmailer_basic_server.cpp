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

#define PORT 8888
#define BUF 1024

using namespace std;

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
    
    for(int i = 0; i < 10; i++){
        char buffer[BUF];
        int bytesRead = recv(clientSocket,buffer,sizeof(buffer),0);

        if(bytesRead == -1){
            cerr << "Error occoured while receiving data from client" << endl;
        }else{
            buffer[bytesRead] = '\0';
            cout << buffer << endl;
        }
    }

    close(clientSocket);
    close(serverSocket);

    return EXIT_SUCCESS;
}