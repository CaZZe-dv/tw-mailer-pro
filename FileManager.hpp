#pragma once

#include <string>
#include <vector>

namespace TwmailerPro{
    //Class for managing everything with files and also creating responses to be sent to client
    class FileManager {
    private:
        //Path for the mailspool directory
        std::string mailboxPath;
        //Method to read specific amount of lines to a given file as path or -1 to read the whole file
        std::vector<std::string> readFile(const std::string& path, const int amount);
        //Method to write into file given as path with specified lines in for mof vector
        //doesnt append lines overrides when theres already something in file
        bool writeFile(const std::string& path, std::vector<std::string> lines);
        //Methode to create a directory by given path, returns true when directory successfully created, false when already exists
        bool createDirectory(const std::string& path);
        //Creating an index file to given path, to track number of next message to be created
        bool createIndexFile(const std::string& path);
        //Method to increment the number in index file
        bool incrementIndexFile(const std::string& path);
        //Fetches the current index of index file 
        int getCurrentIndex(const std::string& path);
    public:
        FileManager(std::string mailboxPath);
        //Creates file in receiver directory with all contents passed to function
        bool createMessage(std::string sender, std::string receiver, std::string subject, std::string message);
        //Lists all message subjects of given user
        std::string listMessages(std::string username);
        //Reads the full content of message from given user and message number
        std::string readMessage(std::string username, int messageNumber);
        //Deletes a specific message of directory of given user and number
        std::string delMessage(std::string username, int messageNumber);
    };
}
