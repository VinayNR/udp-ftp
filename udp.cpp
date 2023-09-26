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

int sendUDPPacket(int sockfd, UDP_PACKET & packet, const struct sockaddr * remoteAddress) {
    socklen_t serverlen = sizeof(struct sockaddr);

    // create the packet buffer data that is sent over the network
    char packet_data[MAX_MSG_SIZE];
    memset(packet_data, 0, sizeof(packet_data));

    // serialize the data
    serialize(packet, packet_data);

    // Send the packet
    int bytesSent = sendto(sockfd, packet_data, strlen(packet_data), 0, remoteAddress, serverlen);

    return bytesSent;
}

int receiveUDPPacket(int sockfd, UDP_PACKET & packet, struct sockaddr * remoteAddress) {
    cout << "Received UDP Packet" << endl;

    socklen_t serverlen = sizeof(struct sockaddr);

    // create the packet buffer data that is filled by the network
    char packet_data[MAX_MSG_SIZE];
    memset(packet_data, 0, sizeof(packet_data));
    
    // Receive a packet
    int bytesReceived = recvfrom(sockfd, packet_data, MAX_MSG_SIZE, 0, remoteAddress, &serverlen);

    cout << "Bytes received: " << bytesReceived << endl;

    // deserialize the packet data
    deserialize(packet_data, packet);

    cout << "Finished desserializing" << endl;

    return bytesReceived;
}


int sendMessage(int sockfd, UDP_MSG * message, const struct sockaddr * remoteAddress) {
    vector<int> failedPackets;
    UDP_MSG * p = message;

    //iterate through the message head pointer to send all packets of the list
    while (p != nullptr) {
        // send the packet pointer by p
        int bytesSent = sendUDPPacket(sockfd, p->packet, remoteAddress);
        if (bytesSent == -1) {
            failedPackets.push_back(p->packet.header.sequence_number);
        }
        p = p->next;
    }

    // retry failed packets

    return 0;
}