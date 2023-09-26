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
#include "udp.h"

using namespace std;


int sendCommand(int sockfd, char * command, const struct sockaddr * remoteAddress) {
    // Create the message that will be serialized and sent over the network
    struct UDP_MSG message;

    // break into packets for transmission and construct a linked message
    UDP_MSG * message_head = nullptr;
    constructMessage(command, message_head);

    cout << "Done constructing message" << endl;

    // send the message
    int status = sendMessage(sockfd, message_head, remoteAddress);
    cout << "Done sending message" << endl;

    if (status != 0) {
        cerr << "Error sending data: " << strerror(errno) << endl;
        return 1;
    }

    return 0;
}

int receiveResponse(int sockfd, char * response, struct sockaddr * remoteAddress) {
    // receive the message until last packet has arrived
    char is_last_packet = 'N';
    UDP_PACKET packet;
    string packet_data;
    cout << "Inside receive response" << endl;
    do {
        int bytesReceived = receiveUDPPacket(sockfd, packet, remoteAddress);

        cout << bytesReceived << " bytes received" << endl;

        packet_data += packet.data;
        
        is_last_packet = packet.header.is_last_packet;

    } while (is_last_packet == 'N');

    // copy the complete data into response
    strcpy(response, packet_data.c_str());

    return 0;
}

int processCommand(int sockfd, char * command, struct sockaddr *& remoteAddress) {

    // send the command
    int status = sendCommand(sockfd, command, remoteAddress);

    cout << "Done send command" << endl;

    if (status == 1) {
        cerr << "Error sending the command" << endl;
        return 1;
    }

    // Server return address
    struct sockaddr_in returnserveraddr;
    socklen_t serverlen = sizeof(struct sockaddr);

    // response set to 0
    char response[MAX_MSG_SIZE];
    memset(response, 0, sizeof(response));

    // Wait for the server to respond
    status = receiveResponse(sockfd, response, (struct sockaddr*)&returnserveraddr);

    cout << response << endl;

    // exit
    if (strcmp(response, "bye") == 0 && strcmp(command, "exit") == 0) {
        return 2;
    }

    if (status == 1) {
        cerr << "Error receving the message" << endl;
        return 1;
    }
    return 0;
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

    struct sockaddr_in *serveraddr = (struct sockaddr_in *)serverinfo->ai_addr;
    void * addr = &(serveraddr->sin_addr);

    char ipstr[INET6_ADDRSTRLEN];
    inet_ntop(serverinfo->ai_family, addr, ipstr, sizeof ipstr);

    cout << "Server address info obtained. IP: " << ipstr << endl << endl;

    cout << "-----UDP based FTP Program-----" << endl;

    // main client loop
    const int commandSize = 1000;
    char command[commandSize];
    
    while (1) {
        cout << endl << "> ";
        if (fgets(command, commandSize, stdin) != nullptr) {

            // remove the trailing \n
            command[strcspn(command, "\n")] = '\0';

            // Process the command
            int status = processCommand(sockfd, command, (sockaddr *&)serveraddr);

            // status 2 is exit successfully
            if (status == 2) {
                break;
            }
        }
    }
    
    return 0;
}