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
#include "file_ops.h"
#include "utils.h"

using namespace std;


int getFileResponseHandler(const char * filename, char *& response, char *& response_type) {
    int fileSize = getFile(filename, response);
    strcpy(response_type, DATA_FLAG);

    // check the status of get
    if (fileSize == -1) {
        delete[] response;
        response = new char[25];
        memset(response, 0, 25);
        strcat(response, "could not get file");
        strcpy(response_type, COMMAND_FLAG);
        return -1;
    }

    // cout << "File size: " << fileSize << endl;
    return fileSize;
}

void putFileResponseHandler(const char * filename, char * fileContents, int dataSize, char *& response) {

    int status = putFile(filename, fileContents, dataSize);

    // allocate memory for response
    response = new char[40];
    memset(response, 0, 40);

    // check the status of creation
    if (status == -1) {
        strcat(response, "could not write file to server");
        return;
    }
    strcat(response, "copied file to server");
}

void deleteFileResponseHandler(const char * filename, char *& response) {
    // delete the file
    int status = deleteFile(filename);

    // allocate memory for response
    response = new char[40];
    memset(response, 0, 40);

    // check the status of deletion
    if (status != 0) {
        strcat(response, "could not delete file on server");
        return;
    }
    strcat(response, "file removed on server");
}

void listFilesResponseHandler(char *& response) {
    const char * currentDir = ".";
    readCurrentDirectory(currentDir, response);
}

void helpResponseHandler(char *& response) {
    response = new char[100];
    memset(response, 0, 100);
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
    // send bye
    response = new char[5];
    memset(response, 0, 5);
    strcpy(response, "bye");
}

void commandNotFoundHandler(char *& response) {
    // send command not found
    response = new char[20];
    memset(response, 0, 20);
    strcpy(response, "command not found");
}

int handleClientRequest(char * command, char * data, int dataSize, char *& response, char *& response_type) {
    // allocate a byte for response_type
    response_type = new char[1];
    memset(response_type, 0, 1);

    // default the response type from the client handler to a command flag
    strcpy(response_type, COMMAND_FLAG);

    // check if the command had one or two words
    char * spacePos = strchr(command, ' ');
    if (spacePos == nullptr) {
        if (strcmp(command, help_command) == 0) {
            // cout << "Help Handler!" << endl;
            helpResponseHandler(response);
        }
        else if (strcmp(command, ls_command) == 0) {
            // cout << "LS Handler!" << endl;
            listFilesResponseHandler(response);
        }
        else if (strcmp(command, exit_command) == 0) {
            // cout << "Exit Handler!" << endl;
            exitResponseHandler(response);
        }
        else {
            commandNotFoundHandler(response);
        }
    }
    else {
        // if space is found, assume second part is the name of a file
        char * filename = new char[strlen(spacePos) + 1];
        strncpy(filename, spacePos+1, strlen(spacePos));

        /*
        for file operations, assume command is the name of file and
        data is the content of file (for put operation)
        */

        if (strncmp(command, get_command, strlen(get_command)) == 0) {
            cout << "Get Handler!" << endl;
            // return the file size obtained from the handler
            int fileSize = getFileResponseHandler(filename, response, response_type);
            return fileSize == -1? strlen(response): fileSize;
        }
        else if (strncmp(command, put_command, strlen(put_command)) == 0) {
            cout << "Put Handler!" << endl;
            putFileResponseHandler(filename, data, dataSize, response);
        }
        else if (strncmp(command, delete_command, strlen(delete_command)) == 0) {
            cout << "Delete Handler!" << endl;
            deleteFileResponseHandler(filename, response);
        }
        else {
            commandNotFoundHandler(response);
        }
    }

    // return the string length of the response message
    return strlen(response);
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
    if ((status = getaddrinfo(NULL, argv[1], &hints, &addressinfo)) != 0) {
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

    /* setsockopt: Handy debugging trick that lets 
   * us rerun the server immediately after we kill it; 
   * otherwise we have to wait about 20 secs. 
   * Eliminates "ERROR on binding: Address already in use" error. 
   */
    int optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));

    cout << "Socket created: " << sockfd << endl;

    // bind the socket
    bind(sockfd, addressinfo->ai_addr, addressinfo->ai_addrlen);

    cout << "Server socket bind complete" << endl;
    cout << "Server listening..." << endl << endl;

    // client's remote address
    struct sockaddr_in remoteAddress;

    // command and data character buffers;
    struct UDP_MSG *udp_message_request = nullptr, *udp_message_response = nullptr;
    char *command = nullptr, *data = nullptr;

    // response from server
    char *response = nullptr, *response_type = nullptr;
    int last_sequence_number;

    int clientResponseSize, clientDataSize;

    // main server loop
    while (true) {

        // free the memory of the linked list
        deleteMessage(udp_message_request);
        deleteMessage(udp_message_response);
        udp_message_request = nullptr;
        udp_message_response = nullptr;

        // free up pointers
        deleteAndNullifyPointer(command, true);
        deleteAndNullifyPointer(data, true);
        deleteAndNullifyPointer(response, true);
        deleteAndNullifyPointer(response_type, true);

        // reset
        clientResponseSize = 0;
        clientDataSize = 0;

        // receive a message (packets) on the socket, and populate the udp_message varaible
        receiveMessage(sockfd, udp_message_request, (struct sockaddr *) &remoteAddress);

        // construct command part and data part from udp_message
        readMessage(udp_message_request, command, &clientResponseSize, data, &clientDataSize);

        // handle the request of the client
        int response_size = handleClientRequest(command, data, clientDataSize, response, response_type);

        // encapsulate the response into a UDP message
        last_sequence_number = writeMessage(response, response_size, response_type, udp_message_response, NEW_MSG_MODE);

        // send the message (packets) on the socket
        sendMessage(sockfd, udp_message_response, last_sequence_number, (struct sockaddr *) &remoteAddress);
    }
    
    return 0;
}