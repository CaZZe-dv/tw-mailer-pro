//All the libraries that are needed for the following program
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
#include <sstream>
#include <functional>
#include <unordered_map>
#include <termios.h>
//Define const variable for buffer received by socket
const int BUFFER_SIZE = 1024;
bool inputIsEmpty(const std::string& input, bool isMultiline){
    std::istringstream stream(input);
    std::string line;
    getline(stream,line);
    if(isMultiline){
        while(line != "."){
            return !(line.length() != 0);
        }
        return true;
    }else{
        return line.length() == 0;
    }
}
//Define method that lets us easily read a specific amoutn of characters or multiline no limitation
//from console, with a hint message. 
//Example: 
//Sender (max. 8 characters): Cazze 
std::string readFromConsole(const std::string& hint, int characters, bool canBeEmpty) {
    std::string input;
    if (characters > 0) {
        std::cout << hint << "(max. " << characters << " characters, " << (canBeEmpty ? "optional" : "required") <<"): ";
        getline(std::cin, input);
        input.append("\n");
    } else if (characters == -1) {
        std::cout << hint << "(no limit, multiline, " << (canBeEmpty ? "optional" : "required") << "):" << std::endl;
        std::string helper;
        do {
            getline(std::cin, helper);
            input.append(helper + "\n");
        } while (helper != ".");
    }
    if(!canBeEmpty && inputIsEmpty(input, (characters == -1 ? true : false))){
        return readFromConsole(hint,characters,canBeEmpty);
    }
    return input;
}
//Function to interfere with input in conole to make it scensored
int getch()
{
    int ch;
    struct termios t_old, t_new;
    tcgetattr(STDIN_FILENO, &t_old);
    t_new = t_old;
    t_new.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &t_new);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &t_old);
    return ch;
}
std::string readFromConsoleHidden(const std::string& hint, int characters, bool canBeEmpty){
    char password[characters];
    int index = 0;
    if(characters > 0){
        std::cout << hint << "(max. " << characters << " characters, " << (canBeEmpty ? "optional" : "required") <<"): ";
        while (true) {
            char ch = getch();  // Use _getch() to read a character without echoing
            if (ch == '\r' || ch == '\n') {
                // If Enter key is pressed, break the loop
                password[index] = '\0';  // Null-terminate the password string
                break;
            } else if (ch == '\b' && index > 0) {
                // If Backspace key is pressed, erase the last character
                std::cout << "\b \b";
                index--;
            } else if (index < sizeof(password) - 1) {
                // Display an asterisk and store the character in the password array
                std::cout << '*';
                password[index] = ch;
                index++;
            }
        }
    }
    if(!canBeEmpty && inputIsEmpty(password,false)){
        return readFromConsoleHidden(hint,characters,canBeEmpty);
    }
    return password;
}
//Create quit protocol to send to the server that client is disconnecting
//Example: 
//QUIT\n
std::string createQuitProtocol() {
    std::string message;
    message.append("QUIT\n");
    return message;
}
//Create delete protocol to be sent to the server to delete specific message of user with given message-number
//Example: 
//DEL\n
//Username: Cazze
//Message-Number: 3
std::string createDelProtocol() {
    std::string message;
    message.append("DEL\n");
    message.append(readFromConsole("Message-Number: ", 10, false));
    return message;
}
//Create read protocol to be sent to server that reads a specific message from a user
//Example:
//READ\n
//Username: Cazze
//Message-Number: 5
std::string createReadProtocol() {
    std::string message;
    message.append("READ\n");
    message.append(readFromConsole("Message-Number: ", 10, false));
    return message;
}
//Create list protocol to be sent to the server that lists a abbreviation of all messages in inbox
//Example:
//LIST\n
//Username: Cazze
std::string createListProtocol() {
    std::string message;
    message.append("LIST\n");
    return message;
}
//Create send protocol to be sent to the server that sends a message to a receiver specified
//Example:
//SEND\n
//Sender: Cazze
//Receiver: Boeck
//Subject: Important topic
//Message:
//This is a very important
//message please read
//it
//.
std::string createSendProtocol() {
    std::string message;
    message.append("SEND\n");
    message.append(readFromConsole("Receiver: ", 8, false));
    message.append(readFromConsole("Subject: ", 80, true));
    message.append(readFromConsole("Message: ", -1, true));
    return message;
}

std::string createLoginProtocol(){
    std::string message;
    message.append("LOGIN\n");
    message.append(readFromConsole("Username: ",20,false));
    message.append(readFromConsoleHidden("Password: ",20,false));
    return message;
}
//Function to handle the transmitting of message to the server, clientsocket and message are passed
//if error occours while sending, error is printed and returns false, otherwise true
bool sendMessageToServer(const int& clientSocket, const std::string& message) {
    int sendStatus = send(clientSocket, message.c_str(), message.length(), 0);
    if (sendStatus == -1) {
        std::cerr << "Error occurred while sending message" << std::endl;
        return false;
    }
    return true;
}
//Function to receive message from server, clientsocket and buffer size are passed
//if error occours while receiving messaage, error is printed and empty string is returned
//otherwise message is returned
std::string receiveMessageFromServer(const int& clientSocket, const int bufferSize) {
    char buffer[bufferSize];
    int bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    if (bytesRead == -1) {
        std::cerr << "Error occurred while receiving data from client" << std::endl;
        return "";    } else {
        buffer[bytesRead] = '\0';
    }
    return buffer;
}
//Prints right usage of programm
void printUsage(const char* programmname){
    std::cerr << "Usage: " << programmname << " <IP_ADDRESS> <PORT>" << std::endl;
}
//Creates connection with server, ip, port and variable for client socket are passed
//clientsocket then is given a value, if anything goes wrong when trying to connect error
//is printed and false returned;
bool establishConnection(const char* ipAdress, const short port, int& clientSocket){
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (clientSocket == -1) {
        std::cerr << "Error occurred while creating client socket" << std::endl;
        return false;
    }

    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    inet_pton(AF_INET, ipAdress, &serverAddress.sin_addr);

    int connectionStatus = connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));

    if (connectionStatus == -1) {
        std::cerr << "Error occurred while trying to connect to server" << std::endl;
        close(clientSocket);
        return false;
    }
    return true;
}

int main(int argc, char* argv[]) {
    //If argument count something other than 3 usage is beeing printed and program ends with error code
    if (argc != 3) {
        char* PROGRAMMNAME = argv[0];
        printUsage(PROGRAMMNAME);
        return EXIT_FAILURE;
    }
    //Save ip adress and port beeing passed in console
    const char* IP_ADDRESS = argv[1];
    const ushort PORT = atoi(argv[2]);
    //Define clientSocket
    int clientSocket;
    //Pass ip adress and port to function
    if(!establishConnection(IP_ADDRESS,PORT,clientSocket)){
        return EXIT_FAILURE;
    }
    //Strings for user input and protocol to be sent to server
    std::string input, protocol, output;
    std::unordered_map<std::string, std::function<std::string()>> protocols;
    protocols["SEND"] = createSendProtocol;
    protocols["LIST"] = createListProtocol;
    protocols["READ"] = createReadProtocol;
    protocols["DEL"] = createDelProtocol;
    protocols["QUIT"] = createQuitProtocol;
    //protocols["LOGIN"] = createLoginProtocol;
    //Login until successful or User quitting
    do {
        std::cout << "You can either login or quit [LOGIN, QUIT]: ";
        getline(std::cin, input);
        if (input == "LOGIN") {
            std::cout << "Enter your LDAP Credentials" << std::endl;
            protocol = createLoginProtocol();
        } else if (input == "QUIT") {
            protocol = createQuitProtocol();
            if (!sendMessageToServer(clientSocket, protocol)) {
                std::cerr << "Couldn't transmit protocol to the server" << std::endl;
            }
            close(clientSocket);
            return EXIT_SUCCESS;
        } else {
            std::cout << "Invalid command has been entered" << std::endl;
            continue;
        }
        if (!sendMessageToServer(clientSocket, protocol)) {
            std::cerr << "Couldn't transmit protocol to the server" << std::endl;
        }
        output = receiveMessageFromServer(clientSocket, BUFFER_SIZE);
        std::cout << output << std::endl;
    } while (output != "OK\n");
    
    //Do as long as user types QUIT
    do {
        std::cout << "Enter your command to be sent to the server [SEND, LIST, READ, DEL, QUIT]: ";
        getline(std::cin, input);
        if(protocols.find(input) != protocols.end()){
            protocol = protocols[input]();
        }else{
            std::cout << "Invalid command has been entered" << std::endl;
            continue;
        }
        //Sending created protocol to server
        if (!sendMessageToServer(clientSocket, protocol)) {
            std::cerr << "Couldn't transmit protocol to server" << std::endl;
        }
        //After that wait for response of server and print to console
        std::cout << receiveMessageFromServer(clientSocket, BUFFER_SIZE) << std::endl;
    } while (input != "QUIT");
    //Close client socket
    close(clientSocket);

    return EXIT_SUCCESS;
}