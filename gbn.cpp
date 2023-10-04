#include "gbn.h"
#include "message.h"
#include "sockutils.h"
#include "utils.h"

#include <iostream>
#include <stdexcept>

using namespace std;

const struct UDP_MSG * sendWindow(const struct UDP_MSG *start, int sockfd, const struct sockaddr *remoteAddress) {
    // send frames of packets starting from the start pointer on the message
    // send upto GBN_N packets as defined in this file
    cout << endl << "----------------------------------------" << endl;
    cout << "In send window with starting seq number: " << start->packet.header.sequence_number << endl;
    const struct UDP_MSG *current = start;

    int bytesSent;

    // iterate until GBN_N packets are sent or the end of list is reached (whichever is first)
    int packets_sent = 0;
    while (current != nullptr && packets_sent < GBN_VALUE) {
        cout << "----------------------------------------" << endl;
        // send the packet pointed to by the current index
        bytesSent = sendUDPPacket(sockfd, &(current->packet), remoteAddress);
        // cout << "Data size in window: " << current->packet.header.dataSize << endl;

        if (bytesSent == -1) {
            // resend the packet
            continue;
        }
        cout << "Sequence number sent: " << current->packet.header.sequence_number << endl;
        cout << "Bytes sent: " << bytesSent << endl;

        // increment counter variables
        ++packets_sent;
        // cout << "Packets sent: " << packets_sent << endl;
        current = current->next;
    }

    // return the pointer to the first unsent packet -> this could be nullptr if all packets were sent
    return current;
}

void receiveWindow(uint32_t expected_sequence_number, struct UDP_MSG *& window_start, struct UDP_MSG *& window_end, int sockfd, struct sockaddr *remoteAddress) {

    cout << endl << "----------------------------------------" << endl;
    cout << "In receive window with expected_sequence_number: " << expected_sequence_number << endl;
    // cout << "In receive window" << endl;
    // packet is set by the receive UDP packet function call
    struct UDP_PACKET *packet = nullptr;
    int bytesReceived, successful_packets = 0;

    struct UDP_MSG *message = nullptr, *current = nullptr, *previous = nullptr;

    bool duplicate_packet = false;

    int incorrect_sequenced_packets = 0;

    while (true) {
        // reset the flag for duplicate
        duplicate_packet = false;
        cout << "----------------------------------------" << endl;

        // wait for packets to arrive from the sender
        // accept packets only if their sequence number is in the accepted range (expected_sequence_number, expected_sequence_number + GBN_N - 1)
        bytesReceived = receiveUDPPacket(sockfd, packet, remoteAddress);

        cout << "Sequence number received: " << packet->header.sequence_number << endl;
        cout << "Bytes received: " << bytesReceived << endl;
        

        // reject the packet if no data received, or if checksum fails, or if received a different packet
        if (bytesReceived == -1
        || !validateChecksum(packet->data, packet->header.dataSize, packet->header.checksum)
        || (packet->header.flag != *COMMAND_FLAG && packet->header.flag != *DATA_FLAG)) {
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
            cout << "Incorrect sequence number of packet was expecting: " << expected_sequence_number << endl;
            ++incorrect_sequenced_packets;

            // trigger an exception to server indicating they are not in sync of the window
            if (incorrect_sequenced_packets >= GBN_VALUE) {
                sendException(expected_sequence_number, sockfd, remoteAddress);
                incorrect_sequenced_packets = 0;
            }
            continue;
        }

        cout << "Packet accpeted" << endl;

        // construct a message
        message = new UDP_MSG;
        message->packet = *packet;
        message->next = nullptr;

        cout << "Message constructed" << endl;

        // delete the packet from memory as it's job is done
        delete packet;
        packet = nullptr;

        cout << "Packet deleted" << endl;

        // insert message into the linked list
        if (window_start == nullptr) {
            window_start = message;
            window_end = message;
            cout << "First message of window" << endl;
        }
        else {
            // find position of insertion
            current = window_start;
            previous = nullptr;
            while (current != nullptr) {
                if (current->packet.header.sequence_number == message->packet.header.sequence_number) {
                    // if the current packet sequence number is found in message, its a duplicate
                    cout << "Continuing due to packet already received" << endl;
                    duplicate_packet = true;
                    break;
                }
                if (current->packet.header.sequence_number > message->packet.header.sequence_number) {
                    // found the first packet, whose sequence number is greater than the message's sequence number
                    cout << "Breaking: found the position to insert packet" << endl;
                    break;
                }
                previous = current;
                current = current->next;
            }

            // continue if the packet is a duplicate (based on sequence number)
            if (duplicate_packet) {
                continue;
            }

            // current is the position to insert the message at
            message->next = current;
            // check if the message was inserted at the last position
            if (message->next == nullptr) {
                cout << "Last message of window" << endl;
                window_end = message;
            }

            // check if the message was inserted at the first position
            if (previous == nullptr) {
                window_start = message;
            }
            else {
                previous->next = message;
            }
        }

        // increment successful packets count
        ++successful_packets;

        // if the window end packet is the last packet of the entire message and we have received all prior ones, break
        // cout << "Considering whether to stop receiving more packets:" << endl;
        // cout << "Window end seq number: " << window_end->packet.header.sequence_number << endl;
        // cout << "Expected seq number start: " << expected_sequence_number << endl;
        // cout << "successful_packets: " << successful_packets << endl;
        // cout << "Is last packet of window, the last one: " << window_end->packet.header.is_last_packet << endl;
        if (window_end->packet.header.is_last_packet == 'Y'
        && window_end->packet.header.sequence_number - expected_sequence_number == successful_packets - 1) {
            cout << "Receive window finished due to last packet received" << endl;
            break;
        }

        // if success_packets have exceeded or equalled to the window length value, break
        if (successful_packets >= GBN_VALUE) {
            cout << "Receive window finished due to GBN value reached" << endl;
            break;
        }
    }
}

void sendException(uint32_t expected_start_sequence_number, int sockfd, const struct sockaddr *remoteAddress) {
    cout << endl << "----------------------------------------" << endl;
    cout << "In send excpetion (with seq number): " << expected_start_sequence_number << endl;

    // construct an exception packet
    struct UDP_PACKET *excp_packet = constructSimplePacket(expected_start_sequence_number, *EXCP_FLAG, "EXC");

    // send this packet out on the network
    int bytesSent;
    do {
        bytesSent = sendUDPPacket(sockfd, excp_packet, remoteAddress);
    } while (bytesSent == -1);

    cout << "Excpetion Sent, requesting for seq number: " << expected_start_sequence_number << endl;

    // clean up
    delete excp_packet;
    excp_packet = nullptr;   
}

void sendAck(uint32_t last_received_sequence_number, int sockfd, const struct sockaddr *remoteAddress) {
    cout << endl << "----------------------------------------" << endl;
    cout << "In send ACK (with ack number): " << last_received_sequence_number << endl;

    // construct an ACK packet
    struct UDP_PACKET *ack_packet = constructSimplePacket(last_received_sequence_number, *ACK_FLAG, "ACK");

    // send this packet out on the network
    int bytesSent;
    do {
        bytesSent = sendUDPPacket(sockfd, ack_packet, remoteAddress);
    } while (bytesSent == -1);

    cout << "ACK Sent for sequence: " << last_received_sequence_number << endl;

    // clean up
    delete ack_packet;
    ack_packet = nullptr;
}

int receiveAck(uint32_t window_last_sequence_number, int sockfd) {
    cout << endl << "----------------------------------------" << endl;
    cout << "In receive ACK" << endl;
    // variable to hold the sender's address
    struct sockaddr *remoteAddress = nullptr;

    // ack_packet is set by the receive UDP packet function call
    struct UDP_PACKET *ack_packet = nullptr;
    int bytesReceived;

    // set timeout for the socket recvfrom
    setSocketTimeout(sockfd, RTT);

    while (true) {
        // wait for acknowledgement from the server
        bytesReceived = receiveUDPPacket(sockfd, ack_packet, remoteAddress);

        // reset the socket wait time
        setSocketTimeout(sockfd, 0);

        // if nothing is received, or packet was deformed
        if (bytesReceived == -1) {
            cout << "Didn't get ACK, resending entire window" << endl;
            return -1;
        }

        if (ack_packet->header.flag == *ACK_FLAG) {
            // check for the ack packet sequence number
            if (window_last_sequence_number == ack_packet->header.sequence_number) {
                // reset socket timeout to 0, indicating no timeout
                cout << "ACK Received for sequence: " << window_last_sequence_number << endl;
                return 0;
            }
            else {
                cout << "Received wrong ACK, number received: " << ack_packet->header.sequence_number << endl;
                return -1;
            }
        }
        else if (ack_packet->header.flag == *EXCP_FLAG) {
            // received an exception flag, throw an excpetion with the sequence number obtained
            throw ack_packet->header.sequence_number;
        }
    }
    return 0;
}