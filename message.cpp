#include "message.h"
#include "utils.h"
#include "gbn.h"

#include <cstdio>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <cerrno>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>

using namespace std;

const int MAX_MSG_SIZE = 1024;
const int MAX_DATA_SIZE = 980; // adjust this

const char * COMMAND_FLAG = "C";
const char * DATA_FLAG = "D";
const char * ACK_FLAG = "A";

const char * APPEND_MSG_MODE = "A";
const char * NEW_MSG_MODE = "N";

/*
Serialization Format
<sequence_number>#<flag>#<is_last_packet>#<checksum>#<data>
*/
int serialize(const struct UDP_PACKET * packet, char *& packet_buffer) {
    
    // allocate memory for the packet_buffer
    packet_buffer = new char[MAX_MSG_SIZE];
    packet_buffer[0] = '\0';

    // Sequence number serialization
    sprintf(packet_buffer, "%d", packet->header.sequence_number);

    // Delimiter
    packet_buffer[strlen(packet_buffer)] = '#';

    // Command or data flag
    packet_buffer[strlen(packet_buffer)] = packet->header.flag;

    // Delimiter
    packet_buffer[strlen(packet_buffer)] = '#';

    // Last Packet flag
    packet_buffer[strlen(packet_buffer)] = packet->header.is_last_packet;

    // Delimiter
    packet_buffer[strlen(packet_buffer)] = '#';

    // Checksum
    // Convert the uint32_t checksum to a string
    std::string checksumStr = to_string(packet->header.checksum);

    // Concatenate the checksum string with the data
    strcat(packet_buffer, checksumStr.c_str());

    // Delimiter
    packet_buffer[strlen(packet_buffer)] = '#';

    // Data
    strcat(packet_buffer, packet->data);

    return 0;
}

int deserialize(const char * packet_buffer, struct UDP_PACKET *& packet) {
    // temporary copy
    char sequence_number[8];
    char flag[2];
    char is_last_token[2];
    char checksumStr[12];
    char * packet_data = new char[MAX_DATA_SIZE];
    packet_data[0] = '\0';

    vector<int> allDelimiterPos = getAllDelimiterPos(packet_buffer, '#');

    // check format
    if (allDelimiterPos.size() != 4) {
        return 1;
    }

    cout << "Size of packet_buffer: " << strlen(packet_buffer) << endl;

    strncpy(sequence_number, packet_buffer, allDelimiterPos[0]);
    strncpy(flag, packet_buffer + allDelimiterPos[0]+1, allDelimiterPos[1] - allDelimiterPos[0] - 1);
    strncpy(is_last_token, packet_buffer + allDelimiterPos[1]+1, allDelimiterPos[2] - allDelimiterPos[1] - 1);
    strncpy(checksumStr, packet_buffer + allDelimiterPos[2]+1, allDelimiterPos[3] - allDelimiterPos[2] - 1);
    strncpy(packet_data, packet_buffer + allDelimiterPos[3]+1, strlen(packet_buffer) - allDelimiterPos[3] - 1);

    // dynamic memory allocation
    packet = new UDP_PACKET;
    memset(packet, 0, sizeof(UDP_PACKET));

    packet->header.sequence_number = atoi(sequence_number);
    packet->header.flag = flag[0];
    packet->header.is_last_packet = is_last_token[0];

    char* endptr;
    packet->header.checksum = strtoul(checksumStr, &endptr, 10);

    strcpy(packet->data, packet_data);
    cout << "Obtained packet size: " << strlen(packet_data) << endl;
    cout << "Packet Size (in deserialize): " << strlen(packet->data) << endl;

    // clean up pointers
    delete[] packet_data;
    packet_data = nullptr;

    return 0;
}

void writeMessage(char * data, const char * flag, UDP_MSG *& message_head, const char * msg_write_mode) {
    UDP_MSG * message_tail;
    int packet_sequence_number = 1;
    int dataSize = strlen(data);

    // calculate the tail pointer position
    if (msg_write_mode == NEW_MSG_MODE) {
        message_head = nullptr;
        message_tail = nullptr;
    }
    else if (msg_write_mode == APPEND_MSG_MODE) {
        if (message_head == nullptr) {
            // head cannot be null in append mode
            return;
        }
        UDP_MSG * current = message_head;
        message_tail = current;
        while (current->next != nullptr) {
            current = current->next;
            message_tail = current;
        }

        // change is_last_packet of tail to 'N' and get the sequence number
        packet_sequence_number = message_tail->packet.header.sequence_number + 1;
        message_tail->packet.header.is_last_packet = 'N';
    }
    
    UDP_MSG * udp_msg;
    for (int i=0; i<dataSize; i+=MAX_DATA_SIZE) {
        udp_msg = new UDP_MSG;
        memset(udp_msg, 0, sizeof(UDP_MSG));
        
        int chunkSize = min(MAX_DATA_SIZE, dataSize - i);

        // set header
        udp_msg->packet.header.sequence_number = packet_sequence_number++;
        udp_msg->packet.header.flag = *flag;
        udp_msg->packet.header.is_last_packet = 'N';

        // copy data
        memcpy(udp_msg->packet.data, data + i, chunkSize);
        // udp_msg->packet.data[chunkSize] = '\0';

        // calculate checksum and add to header
        udp_msg->packet.header.checksum = calculateChecksum(udp_msg->packet.data, chunkSize);

        udp_msg->next = nullptr;

        if (message_head == nullptr) {
            message_head = udp_msg;
            message_tail = udp_msg;
        }
        else {
            message_tail->next = udp_msg;
            message_tail = udp_msg;
        }
    }
    // set the last packet flag to true
    message_tail->packet.header.is_last_packet = 'Y';
}

void readMessage(const struct UDP_MSG * message, char *& command, char *& data) {
    const struct UDP_MSG * ptr = message;

    int command_size = 0, data_size = 0;

    // calculate total size of the char buffers needed
    while (ptr != nullptr) {
        if (ptr->packet.header.flag == *COMMAND_FLAG) {
            cout << "Packet size: " << strlen(ptr->packet.data) << endl;
            command_size += strlen(ptr->packet.data);
        }
        else if (ptr->packet.header.flag == *DATA_FLAG) {
            data_size += strlen(ptr->packet.data);
        }
        ptr = ptr->next;
    }

    // reset and begin to copy
    cout << "Command size: " << command_size << endl;
    if (command_size > 0) {
        command = new char[command_size + 1];
        command[0] = '\0';
    }
    if (data_size > 0) {
        data = new char[data_size + 1];
        data[0] = '\0';
    }
    
    ptr = message;

    while (ptr != nullptr) {
        if (ptr->packet.header.flag == *COMMAND_FLAG) {
            strcat(command, ptr->packet.data);
        }
        else if (ptr->packet.header.flag == *DATA_FLAG) {
            strcat(data, ptr->packet.data);
        }
        ptr = ptr->next;
    }
}

void deleteMessage(struct UDP_MSG * message_head) {
    struct UDP_MSG * current = message_head, * next;
    while (current != nullptr) {
        next = current->next;
        delete current;
        current = next;
    }
}

int sendUDPPacket(int sockfd, const struct UDP_PACKET * packet, const struct sockaddr * remoteAddress) {
    socklen_t serverlen = sizeof(struct sockaddr);

    // create the packet buffer data that is sent over the network
    char * packet_data = nullptr;

    // serialize the data
    // cout << "In send udp packet: " << strlen(packet->data) << endl;
    int status = serialize(packet, packet_data);

    if (status != 0) {
        return -1;
    }

    // Send the packet
    cout << "Sending data: " << packet_data << " : " << strlen(packet_data) << endl;
    int bytesSent = sendto(sockfd, packet_data, strlen(packet_data), 0, remoteAddress, serverlen);
    cout << "Bytes sent: " << bytesSent<< endl;

    // clean up pointers
    delete[] packet_data;
    packet_data = nullptr;

    return bytesSent;
}

int receiveUDPPacket(int sockfd, UDP_PACKET *& packet, struct sockaddr * remoteAddress, int timeout = 0) {
    // set timeout for the recvfrom call if timeout is a positive value. usedful in receiving ack
    if (timeout > 0) {
        struct timeval tv;
        tv.tv_sec = timeout;
        tv.tv_usec = 0;

        // Set the socket option for a receive timeout
        if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == -1) {
            std::cerr << "Error setting socket option" << std::endl;
            close(sockfd);
            return 1;
        }
    }
    
    socklen_t serverlen = sizeof(struct sockaddr);

    // create the packet buffer data that is filled by the network
    char * packet_data = new char[MAX_MSG_SIZE];
    packet_data[0] = '\0';
    
    // Receive a packet
    int bytesReceived = recvfrom(sockfd, packet_data, MAX_MSG_SIZE, 0, remoteAddress, &serverlen);

    cout << "Received data: " << packet_data << " : " << strlen(packet_data) << endl;
    cout << "Bytes received: " << bytesReceived << endl;

    // deserialize the packet data
    int status = deserialize(packet_data, packet);

    if (status != 0) {
        return -1;
    }

    // clean up pointers
    delete[] packet_data;
    packet_data = nullptr;

    return bytesReceived;
}

// All send message operations are successful, upon the last ACK from the receiver
int sendMessage(int sockfd, const struct UDP_MSG *message, uint16_t last_sequence_number, const struct sockaddr *remoteAddress) {

    const struct UDP_MSG *current_window_start = message, *next_window_start = nullptr;
    int ack_status, expected_ack_number;

    while (current_window_start != nullptr) {
        // send the window of packets starting from the current_window_start pointer
        next_window_start = sendWindow(current_window_start, sockfd, remoteAddress);

        // if next_window_start is nullptr, then it was the last window of the message
        expected_ack_number = (next_window_start == nullptr)? last_sequence_number: next_window_start->packet.header.sequence_number - 1;
        
        // wait for acknowledgement of expected_ack_number from the receiver
        ack_status = receiveAck(expected_ack_number, sockfd);

        // if the status of acknowledgement is 1, it is a fail. retransmit the entire window
        if (ack_status == 1) {
            continue;
        }

        // update the current_window_start pointer for the next iteration
        current_window_start = next_window_start;
    }

    return 0;
}

int receiveMessage(int sockfd, struct UDP_MSG *& message_head, struct sockaddr *remoteAddress) {
    struct UDP_PACKET *packet = nullptr;

    // reset head and tail of the linked message
    message_head = nullptr;
    struct UDP_MSG *message_tail = nullptr;

    // message is needed for each packet to be encapsulated
    UDP_MSG *message = nullptr;

    int bytesReceived;

    do {
        // receive a packet
        bytesReceived = receiveUDPPacket(sockfd, packet, remoteAddress);

        // put the packet into a new message
        message = new UDP_MSG;
        cout << "Packet size (in receive message): " << strlen(packet->data) << endl;
        message->packet = *packet;
        message->next = nullptr;

        // add to the two pointer linked list
        if (message_head == nullptr) {
            message_head = message;
            message_tail = message;
        }
        else {
            message_tail->next = message;
            message_tail = message;
        }

    } while (message_tail->packet.header.is_last_packet == 'N');

    return 0;
}