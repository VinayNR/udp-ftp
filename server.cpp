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
#include "udp.h"
#include "file_ops.h"

using namespace std;


void getFileResponseHandler(int sockfd, const struct sockaddr * remoteAddress, const char * filename, char *& response) {
    int status = getFile(filename, response);

    if (status != 0) {
        strcpy(response, "could not get file");
        return;
    }

    // break into packets for transmission
    UDP_MSG * head = nullptr;
    constructMessage(response, head);

    // ready to send
    sendMessage(sockfd, head, remoteAddress);
}

void receiveFileLoop(int sockfd, struct sockaddr * remoteAddress, char *& fileContents) {
    // loop until all packets are received from client
}

void putFileResponseHandler(int sockfd, struct sockaddr * remoteAddress, const char * filename, char *& response) {

    char * fileContents = nullptr;
    // Wait for client to send file packets
    receiveFileLoop(sockfd, remoteAddress, fileContents);

    int status = putFile(filename, fileContents);

    // allocate memory for response
    response = new char[50];
    // memset(response, 0, sizeof(response));
    if (status != 0) {
        strcpy(response, "could not write file to server");
        return;
    }
    strcpy(response, "copied file to server");
}

void deleteFileResponseHandler(const char * filename, char *& response) {
    // delete the file
    int status = deleteFile(filename);

    // set memory for the output parameter response
    response = new char[50];

    // check the status of deletion
    if (status != 0) {
        strcpy(response, "could not delete file");
        return;
    }
    strcpy(response, "file removed");
}

void listFilesResponseHandler(char *& response) {
    const char * currentDir = ".";
    readCurrentDirectory(currentDir, response);
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

void handleClientRequest(int sockfd, struct sockaddr * remoteAddress, struct UDP_PACKET & clientMessage, char *& response) {

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
            getFileResponseHandler(sockfd, remoteAddress, filename, response);
        }
        else if (strncmp(request, put_command, strlen(put_command)) == 0) {
            cout << "Put Handler!" << endl;
            putFileResponseHandler(sockfd, remoteAddress, filename, response);
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
    char packet_data[MAX_MSG_SIZE];
    
    int bytesSent = 0, bytesReceived = 0;

    while (1) {

        // process packets
        memset(packet_data, 0, sizeof(packet_data));
        bytesReceived = recvfrom(sockfd, packet_data, MAX_MSG_SIZE, 0, (struct sockaddr *) &clientaddr, &clientlen);
        cout << "Bytes received : " << bytesReceived << endl;

        if (bytesReceived == -1) {
            cerr << "Error receiving data: " << strerror(errno) << endl;
        }
        
        // deserialize the message
        struct UDP_PACKET clientMessage;
        deserialize(packet_data, clientMessage);

        cout << "Message from client " << inet_ntoa(clientaddr.sin_addr) << " : " << clientMessage.data << endl;

        // handle the message received and construct a response
        char * response = nullptr;
        handleClientRequest(sockfd, (sockaddr *)&clientaddr, clientMessage, response);

        // send the response back
        bytesSent = sendto(sockfd, response, strlen(response), 0, (struct sockaddr*) &clientaddr, clientlen);

        if (bytesSent == -1) {
            cerr << "Error sending data: " << strerror(errno) << endl;
        }

        cout << "Bytes sent " << bytesSent << endl;
    }
    
    return 0;
}