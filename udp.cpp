#include "udp.h"
#include "message.h"

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


using namespace std;

int sendUDPPacket(int sockfd, UDP_PACKET * packet, const struct sockaddr * remoteAddress) {
    // Send a message using sendto command
    socklen_t serverlen = sizeof(struct sockaddr);
    int bytesSent = sendto(sockfd, packet->data, strlen(packet->data), 0, remoteAddress, serverlen);

    return bytesSent;
}

int receiveUDPPacket(int sockfd, UDP_PACKET * packet, struct sockaddr * remoteAddress) {
    // Receive a message using recvfrom command
    socklen_t serverlen = sizeof(struct sockaddr);
    int bytesReceived = recvfrom(sockfd, packet->data, MAX_MSG_SIZE, 0, remoteAddress, &serverlen);

    return bytesReceived;
}


int sendMessage(int sockfd, UDP_MSG * message, const struct sockaddr * remoteAddress) {
    vector<int> failedPackets;
    UDP_MSG * p = message;
    while (p != nullptr) {
        // send the packet pointer by p
        int bytesSent = sendUDPPacket(sockfd, p->packet, remoteAddress);
        if (bytesSent == -1) {
            failedPackets.push_back(p->packet.header.sequence_number);
        }
    }

    // retry failed packets

    return 0;
}