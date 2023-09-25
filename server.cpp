#include <iostream>
#include <fstream>
#include <cerrno>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>

#include "commands.h"
#include "message.h"

using namespace std;


void getFiles(const char * directoryPath, char *& response) {
    
    DIR * dir = opendir(directoryPath);
    
    if (dir) {
        vector<string> fileNames;

        struct dirent* entry;
        while ((entry = readdir(dir))) {
            if (entry->d_type == DT_REG) { // Check if it's a regular file
                fileNames.push_back(entry->d_name);
            }
        }

        closedir(dir);

        // Create a char buffer and concatenate the file names
        string concatenatedFileNames;
        for (const string& fileName : fileNames) {
            concatenatedFileNames += fileName + '\n';
        }

        // Copy the concatenated file names to a char buffer
        response = new char[concatenatedFileNames.size() + 2];
        strcpy(response, concatenatedFileNames.c_str());
    }
}

void getFile(const char * filename, char *& response) {
    // int fileSize = getFileSize(filename);

    ifstream filestream (filename, ios::binary | ios::ate);
    if (!filestream.is_open()) {
        cerr << "Error opening file: " << filename << endl;
        response = "issue with file";
        return;
    }

    int fileSize = filestream.tellg();
    filestream.seekg(0, ios::beg);

    response = new char[fileSize + 1];

    if (!filestream.read(response, fileSize)) {
        std::cerr << "Error reading file: " << filename << std::endl;
        response = "issue with file";
        return;
    }

    // Null-terminate the string
    response[fileSize] = '\0';

    filestream.close();
}


void getFileResponseHandler(const char * filename, char *& response) {
    getFile(filename, response);
}

void putFileResponseHandler(const char * filename, char *& response) {

}

void deleteFileResponseHandler(const char * filename, char *& response) {
    if (remove(filename) == 0) {
        response = new char[50];
        strcat(response, filename);
        strcat(response, " removed");
    }
}

void listFilesResponseHandler(char *& response) {
    const char * currentDir = ".";
    getFiles(currentDir, response);
}

void helpResponseHandler(char *& response) {
    response = new char[100];
    strcpy(response, help_command);
    strcat(response, " ");
    strcat(response, ls_command);
    strcat(response, " ");
    strcat(response, exit_command);
    strcat(response, " ");
    strcat(response, get_command);
    strcat(response, " ");
    strcat(response, put_command);
    strcat(response, " ");
    strcat(response, delete_command);
}

void exitResponseHandler(char *& response) {
    // send Bye
    response = new char[10];

    const char * bye_message = "bye";
    strcpy(response, bye_message);
}

void commandNotFoundHandler(char *& response) {
    // send command not found
    response = new char[20];
    const char * command_not_found = "command not found";
    strcpy(response, command_not_found);
}

void handleClientRequest(struct udp_msg & clientMessage, char *& response) {

    // parse the client request
    char * request = clientMessage.data;

    char * spacePos = strchr(request, ' ');

    if (spacePos == nullptr) {
        if (strcmp(request, help_command) == 0) {
            cout << "Help Handler!" << endl;
            helpResponseHandler(response);
        }
        else if (strcmp(request, ls_command) == 0) {
            cout << "LS Handler!" << endl;
            listFilesResponseHandler(response);
        }
        else if (strcmp(request, exit_command) == 0) {
            cout << "Exit Handler!" << endl;
            exitResponseHandler(response);
        }
        else {
            commandNotFoundHandler(response);
        }
    }
    else {
        char * filename = new char[strlen(spacePos) + 1];
        strncpy(filename, spacePos+1, strlen(spacePos));

        if (strncmp(request, get_command, strlen(get_command)) == 0) {
            cout << "Get Handler!" << endl;
            getFileResponseHandler(filename, response);
        }
        else if (strncmp(request, put_command, strlen(put_command)) == 0) {
            cout << "Put Handler!" << endl;
            putFileResponseHandler(filename, response);
        }
        else if (strncmp(request, delete_command, strlen(delete_command)) == 0) {
            cout << "Delete Handler!" << endl;
            deleteFileResponseHandler(filename, response);
        }
        else {
            commandNotFoundHandler(response);
        }
    }
    // response[strlen(response)] = '\0';
}

int main(int argc, char * argv[]) {
    // check if usage is corect
    if (argc != 2) {
        cout << "Usage error: <executable> <port>" << endl;
        return 1;
    }

    // Get own address
    int status;
    struct addrinfo hints;
    struct addrinfo *addressinfo;

    // set the hints needed
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // use this to get the own address

    // get the address info
    if ((status = getaddrinfo("localhost", argv[1], &hints, &addressinfo)) != 0) {
        cout << "Error with get addr info: " << gai_strerror(status) << endl;
        return 1;
    }

    cout << "Address information obtained" << endl;

    // server own ip
    void * addr;
    struct sockaddr_in *ipv4 = (struct sockaddr_in *)addressinfo->ai_addr;
    addr = &(ipv4->sin_addr);

    // create a socket
    int sockfd = socket(addressinfo->ai_family, addressinfo->ai_socktype, addressinfo->ai_protocol);

    if (sockfd == -1) {
        cout << "Error creating a socket" << endl;
        return 1;
    }

    cout << "Socket created: " << sockfd << endl;

    // bind the socket
    bind(sockfd, addressinfo->ai_addr, addressinfo->ai_addrlen);

    cout << "Server socket bind complete" << endl;

    struct sockaddr_in clientaddr;
    socklen_t clientlen = sizeof(clientaddr);
    cout << "Server listening..." << endl << endl;

    // main server loop
    char packet[MAX_MSG_SIZE];
    
    int bytesSent = 0, bytesReceived = 0;

    while (1) {
        memset(packet, 0, sizeof(packet));
        bytesReceived = recvfrom(sockfd, packet, MAX_MSG_SIZE, 0, (struct sockaddr *) &clientaddr, &clientlen);
        cout << "Bytes received : " << bytesReceived << endl;

        if (bytesReceived == -1) {
            cerr << "Error receiving data: " << strerror(errno) << endl;
        }
        
        // deserialize the message
        struct udp_msg clientMessage;
        deserialize(packet, clientMessage);

        cout << "Message from client " << inet_ntoa(clientaddr.sin_addr) << " : " << clientMessage.data << endl;

        // handle the message received and construct a response
        char * response = nullptr;
        handleClientRequest(clientMessage, response);

        // cout << response << endl << strlen(response) << endl;

        // send the response back
        bytesSent = sendto(sockfd, response, strlen(response), 0, (struct sockaddr*) &clientaddr, clientlen);

        if (bytesSent == -1) {
            cerr << "Error sending data: " << strerror(errno) << endl;
        }

        cout << "Bytes sent " << bytesSent << endl;
    }
    
    return 0;
}