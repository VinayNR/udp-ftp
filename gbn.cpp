#include "gbn.h"
#include "message.h"

#include <iostream>

using namespace std;

const struct UDP_MSG * sendWindow(const struct UDP_MSG *start, int sockfd, const struct sockaddr *remoteAddress) {
    // send frames of packets starting from the start pointer on the message
    // send upto GBN_N packets as defined in this file
    const struct UDP_MSG *current = start;

    // iterate until GBN_N packets are sent or the end of list is reached (whichever is first)
    int packets_sent = 0;
    while (current != nullptr && packets_sent < GBN_VALUE) {
        // send the packet pointed to by the current index
        int bytesSent = sendUDPPacket(sockfd, &(current->packet), remoteAddress);
        cout << "Bytes sent for Window: " << bytesSent << endl;

        // increment counter variables
        ++packets_sent;
        current = current->next;
    }

    // return the pointer to the first unsent packet -> this could be nullptr if all packets were sent
    return current;
}

void receiveWindow() {

}

void sendAck(int last_received_sequence_number, int sockfd, const struct sockaddr *remoteAddress) {
    // send a udp packet with the ack number in the packet's sequence number field
    // the ack is always a single packet and not a message

    // allocate memory for the ack packet and set it to 0
    struct UDP_PACKET *ack_packet = new UDP_PACKET;
    memset(ack_packet, 0, sizeof(UDP_PACKET));

    // set the various fields of the ACK packet
    ack_packet->header.sequence_number = last_received_sequence_number; // TODO: think about this
    ack_packet->header.flag = *ACK_FLAG;
    ack_packet->header.is_last_packet = 'Y';

    // send this packet out on the network
    int bytesSent = sendUDPPacket(sockfd, ack_packet, remoteAddress);
    cout << "Bytes sent for ACK: " << bytesSent << endl;

    // clean up
    delete ack_packet;
    ack_packet = nullptr;
}

int receiveAck(int window_last_sequence_number, int sockfd) {
    // variable to hold the sender's address
    struct sockaddr *remoteAddress;

    // ack_packet is set by the receive UDP packet function call
    struct UDP_PACKET *ack_packet = nullptr;
    int bytesReceived;

    while (true) {
        // wait for acknowledgement from the server
        bytesReceived = receiveUDPPacket(sockfd, ack_packet, remoteAddress);
        if (bytesReceived == -1) {
            // indicate a failure
            return 1;
        }
        cout << "Bytes received for ACK: " << bytesReceived << endl;

        // check if the ACK packet is the last sequence number expected
        if (window_last_sequence_number == ack_packet->header.sequence_number) {
            return 0;
        }
    }

    
    return 0;
}