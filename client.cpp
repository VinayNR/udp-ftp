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

using namespace std;

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
    struct sockaddr_in returnserveraddr;
    void * addr = &(serveraddr->sin_addr);

    char ipstr[INET6_ADDRSTRLEN];
    inet_ntop(serverinfo->ai_family, addr, ipstr, sizeof ipstr);

    cout << "Server address info obtained. IP: " << ipstr << endl << endl;

    cout << "-----UPD based FTP Program-----" << endl;

    char packet[MAX_MSG_SIZE];

    socklen_t serverlen = sizeof(struct sockaddr);

    // main client loop
    const int commandSize = 1000; // bytes
    char command[commandSize];
    struct udp_msg udp_cmd;

    char response[MAX_MSG_SIZE];

    while (1) {
        cout << endl << "> ";
        if (fgets(command, commandSize, stdin) != nullptr) {
            
            // remove the trailing \n
            size_t newlinePos = strcspn(command, "\n");

            // Replace the newline character with a null terminator
            if (newlinePos < strlen(command)) {
                command[newlinePos] = '\0';
            }

            cout << command << " command" << endl;

            // set the udp command
            udp_cmd.sequence_number = 1;
            strcpy(udp_cmd.data, command);

            int bytesSent = 0, bytesReceived = 0;

            // Copy the struct's data into a char buffer
            serialize(udp_cmd, packet);

            bytesSent = sendto(sockfd, packet, strlen(packet), 0, (struct sockaddr*)serveraddr, serverlen);

            // TODO: do some better error handling
            if (bytesSent == -1) {
                cerr << "Error sending data: " << strerror(errno) << endl;
                close(sockfd);
                return 1;
            }
            cout << "Bytes sent : " << bytesSent << endl;

            // Wait for the server to respond
            memset(response, 0, sizeof(response));
            bytesReceived = recvfrom(sockfd, response, MAX_MSG_SIZE, 0, (struct sockaddr*)&returnserveraddr, &serverlen);
            if (bytesReceived == -1) {
                cerr << "Error receiving data: " << strerror(errno) << endl;
                close(sockfd);
                return 1;
            }
            cout << "Bytes received : " << bytesReceived << endl;

            cout << response << endl;

            // break out if the response is bye
            if (strcmp(response, "bye") == 0 && strcmp(command, "exit") == 0) {
                break;
            }
        }
    }
    
    return 0;
}