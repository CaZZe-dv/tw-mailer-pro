#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 6543
#define BUF 1024

using namespace std;

int main(int argc, char* argv[]){

    int clientSocket = socket(AF_INET,SOCK_STREAM,0);

    if(clientSocket == -1){
        cerr << "Error occoured while creating clientsocket" << endl;
        return EXIT_FAILURE;
    }

    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET; // IPv4
    serverAddress.sin_port = htons(PORT); // Server port (in network byte order)
    inet_pton(AF_INET, "127.0.0.1", &serverAddress.sin_addr);

    int connectionStatus = connect(clientSocket,(struct sockaddr*)&serverAddress,sizeof(serverAddress));

    if(connectionStatus == -1){
        cerr << "Error occoured while trying to connect to server" << endl;
        close(clientSocket);
        return EXIT_FAILURE;
    }

    const char* message = "Hello Server";
    int sendStatus  = send(clientSocket,message,strlen(message),0);
    if(sendStatus == -1){
        cerr << "Error occoured while sending message" << endl;
    }

    close(clientSocket);

    return EXIT_SUCCESS;
}
