#include <iostream>
#include <cerrno>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "message.h"
#include "commands.h"
#include "file_ops.h"
#include "utils.h"

using namespace std;


void printServerMessage(char * message) {
    cout << message << endl;
}

void handleServerData(char * data, char * filename) {
    // save data from server locally
    int status = putFile(filename, data);
    if (status != 0) {
        cout << "failed to copy file" << endl;
        return;
    }
    cout << "file received from server" << endl;
}


int main(int argc, char * argv[]) {
    // check if the command line usage is correct
    if (argc != 3) {
        cout << "Usage error: <executable> <server_address> <port>" << endl;
        return 1;
    }

    // create a sock stream socket
    int sockfd;
    sockfd = socket(PF_INET, SOCK_DGRAM, 0);

    if (sockfd == -1) {
        cout << "Error creating a socket" << endl;
        return 1;
    }

    cout << "Socket created : " << sockfd << endl;

    // get the server address
    char * serverAddress = argv[1];
    char * serverPort = argv[2];

    struct addrinfo hints;
    struct addrinfo *serverinfo;

    // set the hints needed
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    cout << "Server address : " << serverAddress << endl;
    cout << "Server port : " << serverPort << endl;

    // get server address info structures
    int status = getaddrinfo(serverAddress, serverPort, &hints, &serverinfo);
    if (status != 0) {
        cout << "Error with get addr info: " << gai_strerror(status) << endl;
        return 1;
    }

    struct sockaddr_in *remoteAddress = (struct sockaddr_in *)serverinfo->ai_addr;
    void * addr = &(remoteAddress->sin_addr);

    char ipstr[INET6_ADDRSTRLEN];
    inet_ntop(serverinfo->ai_family, addr, ipstr, sizeof ipstr);

    cout << "Server address info obtained. IP: " << ipstr << endl << endl;

    cout << "-----UDP based FTP Program-----" << endl;

    // main client loop
    const int commandSize = 1000;
    char command[commandSize];

    struct UDP_MSG *udp_message_request = nullptr, *udp_message_response = nullptr;

    char * server_response = nullptr, * server_data = nullptr;

    struct sockaddr * receivingRemoteAddress;
    uint32_t last_sequence_number;
    
    while (1) {
        cout << endl << "> ";
        // get the command from user
        if (fgets(command, commandSize, stdin) != nullptr) {

            // remove the trailing \n
            command[strcspn(command, "\n")] = '\0';

            // free the UDP message pointers
            deleteMessage(udp_message_request);
            deleteMessage(udp_message_response);
            udp_message_request = nullptr;
            udp_message_response = nullptr;

            receivingRemoteAddress = nullptr;

            // free the server message pointers
            delete[] server_response;
            delete[] server_data;
            server_response = nullptr;
            server_data = nullptr;

            char * spacePos = strchr(command, ' ');

            if (spacePos == nullptr) {
                if (strcmp(command, help_command) == 0
                || strcmp(command, ls_command) == 0
                || strcmp(command, exit_command) == 0) {
                    // construct a message
                    last_sequence_number = writeMessage(command, COMMAND_FLAG, udp_message_request, NEW_MSG_MODE);

                    cout << "Message: " << udp_message_request->packet.data << endl;
                    cout << "Last Seq number: " << last_sequence_number << endl;

                    // send the message
                    sendMessage(sockfd, udp_message_request, last_sequence_number, (struct sockaddr *)remoteAddress);

                    // wait for the response from server
                    receiveMessage(sockfd, udp_message_response, receivingRemoteAddress);

                    // construct command part and data part from udp_message
                    readMessage(udp_message_response, server_response, server_data);

                    // handle the response from server
                    // printServerMessage(server_response);
                }
            }
            // else {
                // // if space is found, assume second part is the name of a file
                // char * filename = new char[strlen(spacePos) + 1];
                // strncpy(filename, spacePos+1, strlen(spacePos));

                // // delete command
                // if (strncmp(command, delete_command, strlen(delete_command)) == 0) {
                //     // construct a message
                //     writeMessage(command, COMMAND_FLAG, udp_message_request, NEW_MSG_MODE);

                //     // send the message request over the network
                //     sendMessage(sockfd, udp_message_request, (struct sockaddr *) remoteAddress);

                //     // wait for the response from server
                //     receiveMessage(sockfd, udp_message_response, receivingRemoteAddress);

                //     // construct command part and data part from udp_message
                //     readMessage(udp_message_response, server_response, server_data);

                //     // handle the response from server
                //     printServerMessage(server_response);
                // }

                // // get command
                // else if (strncmp(command, get_command, strlen(get_command)) == 0) {
                //     // construct a message
                //     writeMessage(command, COMMAND_FLAG, udp_message_request, NEW_MSG_MODE);

                //     // send the message request over the network
                //     sendMessage(sockfd, udp_message_request, (struct sockaddr *) remoteAddress);

                //     // wait for the response from server
                //     receiveMessage(sockfd, udp_message_response, receivingRemoteAddress);

                //     // construct command part and data part from udp_message
                //     readMessage(udp_message_response, server_response, server_data);

                //     // handle the response from server
                //     if (server_response != nullptr) {
                //         printServerMessage(server_response);
                //     }
                //     if (server_data != nullptr) {
                //         handleServerData(server_data, filename);
                //     }
                // }
                
                // // put command
                // else if (strncmp(command, put_command, strlen(put_command)) == 0) {
                //     // construct the command message
                //     writeMessage(command, COMMAND_FLAG, udp_message_request, NEW_MSG_MODE);

                //     // read the file from local directory
                //     char * fileContents = nullptr;
                //     int status = getFile(filename, fileContents);
                //     if (status != 0) {
                //         cout << "error reading file locally" << endl;
                //         continue;
                //     }

                //     // construct the data message and append it
                //     writeMessage(fileContents, DATA_FLAG, udp_message_request, APPEND_MSG_MODE);

                //     // send the messages request over the network
                //     sendMessage(sockfd, udp_message_request, (struct sockaddr *) remoteAddress);

                //     // wait for the response from server
                //     receiveMessage(sockfd, udp_message_response, receivingRemoteAddress);

                //     // construct command part and data part from udp_message
                //     readMessage(udp_message_response, server_response, server_data);

                //     // handle the response from server
                //     if (server_response != nullptr) {
                //         printServerMessage(server_response);
                //     }
                //     if (server_data != nullptr) {
                //         handleServerData(server_data, filename);
                //     }
                // }
            // }
        }
    }
    
    return 0;
}