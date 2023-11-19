#include "FileManager.hpp"
#include <iostream>
#include <sstream>
#include <fstream>
#include <filesystem>

namespace TwmailerPro{
FileManager::FileManager(std::string mailboxPath) {
    this->mailboxPath = mailboxPath;
    std::cout << "Mail directory path is: " << mailboxPath << std::endl;
}
//Method to read specific amount of lines to a given file as path or -1 to read the whole file
std::vector<std::string> FileManager::readFile(const std::string& path, const int amount) {
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
bool FileManager::writeFile(const std::string& path, std::vector<std::string> lines) {
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
bool FileManager::createDirectory(const std::string& path) {
    if (std::filesystem::create_directory(path)) {
        return true;
    }
    else {
        std::cerr << "User already has an inbox" << std::endl;
        return false;
    }
}
//Creating an index file to given path, to track number of next message to be created
bool FileManager::createIndexFile(const std::string& path) {
    std::vector<std::string> lines = { "1" };
    return writeFile(path, lines);
}
//Method to increment the number in index file
bool FileManager::incrementIndexFile(const std::string& path) {
    int index = std::stoi(readFile(path, 1)[0]);
    std::vector<std::string> lines = { std::to_string(++index) };
    return writeFile(path, lines);
}
//Fetches the current index of index file 
int FileManager::getCurrentIndex(const std::string& path) {
    return std::stoi(readFile(path, 1)[0]);
}

//Creates file in receiver directory with all contents passed to function
bool FileManager::createMessage(std::string sender, std::string receiver, std::string subject, std::string message) {
    std::cout << mailboxPath << " " << sender << " " << receiver << " " << subject << " " << message << std::endl;
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
std::string FileManager::listMessages(std::string username) {
    std::string pathUsername = mailboxPath + "/" + username;
    std::string response, helper;
    int counter = 0;
    if (std::filesystem::is_directory(pathUsername)) {
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
std::string FileManager::readMessage(std::string username, int messageNumber) {
    std::string pathUsername = mailboxPath + "/" + username;
    std::string response;
    int counter = 0;
    if (std::filesystem::is_directory(pathUsername)) {
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
std::string FileManager::delMessage(std::string username, int messageNumber) {
    std::string pathUsername = mailboxPath + "/" + username;
    std::string response;
    int counter = 0;
    if (std::filesystem::is_directory(pathUsername)) {
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
}