#include "gbn.h"
#include "message.h"
#include "sockutils.h"
#include "utils.h"

#include <iostream>

using namespace std;

const struct UDP_MSG * sendWindow(const struct UDP_MSG *start, int sockfd, const struct sockaddr *remoteAddress) {
    // send frames of packets starting from the start pointer on the message
    // send upto GBN_N packets as defined in this file
    cout << "In send window" << endl;
    const struct UDP_MSG *current = start;

    int status;

    // iterate until GBN_N packets are sent or the end of list is reached (whichever is first)
    int packets_sent = 0;
    while (current != nullptr && packets_sent < GBN_VALUE) {
        // send the packet pointed to by the current index
        status = sendUDPPacket(sockfd, &(current->packet), remoteAddress);

        if (status != 0) {
            // resend the packet
            continue;
        }

        // increment counter variables
        ++packets_sent;
        cout << "Packets sent: " << packets_sent << endl;
        current = current->next;
    }

    // return the pointer to the first unsent packet -> this could be nullptr if all packets were sent
    return current;
}

void receiveWindow(uint32_t expected_sequence_number, struct UDP_MSG *& window_start, struct UDP_MSG *& window_end, int sockfd, struct sockaddr *remoteAddress) {
 
    // cout << "In receive window" << endl;
    // packet is set by the receive UDP packet function call
    struct UDP_PACKET *packet = nullptr;
    int status, successful_packets = 0;

    struct UDP_MSG *message = nullptr, *current = nullptr, *previous = nullptr;

    while (true) {
        // wait for packets to arrive from the sender
        // accept packets only if their sequence number is in the accepted range (expected_sequence_number, expected_sequence_number + GBN_N - 1)
        status = receiveUDPPacket(sockfd, packet, remoteAddress);

        if (status != 0) {
            // delete pointers
            delete packet;
            packet = nullptr;
            // the packet was corrupt, discard it
            continue;
        }

        // check if the packet's checksum is the same
        if (!validateChecksum(packet->data, packet->header.checksum)) {
            // delete pointers
            delete packet;
            packet = nullptr;
            // the packet was corrupt, discard it
            continue;
        }

        // check if the sequence number is not in the expected range
        if (packet->header.sequence_number < expected_sequence_number
        || packet->header.sequence_number >= expected_sequence_number + GBN_VALUE) {
            // delete pointers
            delete packet;
            packet = nullptr;
            // wrong packet arrived, discard it
            cout << "Incorrect sequence number of packet" << endl;
            continue;
        }

        // construct a message
        message = new UDP_MSG;
        message->packet = *packet;
        message->next = nullptr;

        // insert message into the linked list
        if (window_start == nullptr) {
            window_start = message;
            window_end = message;
        }
        else {
            // find position of insertion
            current = window_start;
            previous = nullptr;
            while (current != nullptr) {
                if (current->packet.header.sequence_number == message->packet.header.sequence_number) {
                    // if the current packet sequence number is found in message, its a duplicate
                    continue;
                }
                if (current->packet.header.sequence_number > message->packet.header.sequence_number) {
                    // found the first packet, whose sequence number is greater than the message's sequence number
                    break;
                }
                previous = current;
                current = current->next;
            }

            // current is the position to insert the message at
            message->next = current;
            // check if the message was inserted at the last position
            if (message->next == nullptr) {
                window_end = message;
            }

            previous->next = message;
            // check if the message was inserted at the first position
            if (previous == nullptr) {
                window_start = message;
            }
        }

        // increment successful packets count
        ++successful_packets;

        // if the window end packet is the last packet of the entire message and we have received all prior ones, break
        // cout << "Considering whether to stop receiving more packets:" << endl;
        // cout << "Window end seq number: " << window_end->packet.header.sequence_number << endl;
        // cout << "Expected seq number start: " << expected_sequence_number << endl;
        // cout << "successful_packets: " << successful_packets << endl;
        // cout << window_end->packet.header.is_last_packet << endl;
        if (window_end->packet.header.is_last_packet == 'Y'
        && window_end->packet.header.sequence_number - expected_sequence_number == successful_packets - 1) {
            // cout << "Receive window finished due to last packet received" << endl;
            break;
        }

        // if success_packets have exceeded or equalled to the window length value, break
        if (successful_packets >= GBN_VALUE) {
            // cout << "Receive window finished due to GBN value reached" << endl;
            break;
        }
    }
}

void sendAck(uint32_t last_received_sequence_number, int sockfd, const struct sockaddr *remoteAddress) {
    cout << "In send ACK (with ack number): " << last_received_sequence_number << endl;
    // send a udp packet with the ack number in the packet's sequence number field
    // the ack is always a single packet and not a message

    // allocate memory for the ack packet and set it to 0
    struct UDP_PACKET *ack_packet = new UDP_PACKET;
    memset(ack_packet, 0, sizeof(UDP_PACKET));

    // set the various fields of the ACK packet
    ack_packet->header.sequence_number = last_received_sequence_number;
    ack_packet->header.flag = *ACK_FLAG;
    ack_packet->header.is_last_packet = 'Y';

    // data for ACK packet is set to string ACK
    strcpy(ack_packet->data, "ACK");
    ack_packet->header.checksum = djb2_hash(ack_packet->data, strlen(ack_packet->data));
    
    // send this packet out on the network
    int status;
    do {
        status = sendUDPPacket(sockfd, ack_packet, remoteAddress);
    } while (status != 0);

    // clean up
    delete ack_packet;
    ack_packet = nullptr;
}

int receiveAck(uint32_t window_last_sequence_number, int sockfd) {
    cout << "In receive ACK" << endl;
    // variable to hold the sender's address
    struct sockaddr *remoteAddress = nullptr;

    // ack_packet is set by the receive UDP packet function call
    struct UDP_PACKET *ack_packet = nullptr;
    int status;

    // set timeout for the socket recvfrom
    setSocketTimeout(sockfd, RTT);

    while (true) {
        // wait for acknowledgement from the server
        status = receiveUDPPacket(sockfd, ack_packet, remoteAddress);
        cout << "Status received for ACK: " << status << endl;
        if (status == -1) {
            // reset socket timeout to 0, indicating no timeout
            setSocketTimeout(sockfd, 0);
            // indicate a failure
            return -1;
        }
        // check if the ACK packet is the last sequence number expected
        if (window_last_sequence_number == ack_packet->header.sequence_number) {
            // reset socket timeout to 0, indicating no timeout
            setSocketTimeout(sockfd, 0);
            break;
        }
    }
    return 0;
}