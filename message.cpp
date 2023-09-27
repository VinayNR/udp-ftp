#include "message.h"
#include "utils.h"

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

const char * APPEND_MSG_MODE = "A";
const char * NEW_MSG_MODE = "N";

/*
Serialization Format
<sequence_number>#<flag>#<is_last_packet>#<checksum>#<data>
*/
int serialize(struct UDP_PACKET * packet, char * data) {
    
    // Sequence number serialization
    sprintf(data, "%d", packet->header.sequence_number);

    // Delimiter
    data[strlen(data)] = '#';

    // Command or data flag
    data[strlen(data)] = packet->header.flag;

    // Delimiter
    data[strlen(data)] = '#';

    // Last Packet flag
    data[strlen(data)] = packet->header.is_last_packet;

    // Delimiter
    data[strlen(data)] = '#';

    // Checksum
    // Convert the uint32_t checksum to a string
    std::string checksumStr = to_string(packet->header.checksum);

    // Concatenate the checksum string with the data
    strcat(data, checksumStr.c_str());

    // Delimiter
    data[strlen(data)] = '#';

    // Data
    strcat(data, packet->data);

    return 0;
}

int deserialize(char * data, struct UDP_PACKET *& packet) {
    // temporary copy
    char sequence_number[8];
    char flag[2];
    char is_last_token[2];
    char checksumStr[12];
    char packet_data[MAX_DATA_SIZE];

    vector<int> allDelimiterPos = getAllDelimiterPos(data, '#');

    // check format
    if (allDelimiterPos.size() != 4) {
        return 1;
    }

    strncpy(sequence_number, data, allDelimiterPos[0]);
    strncpy(flag, data + allDelimiterPos[0]+1, allDelimiterPos[1] - allDelimiterPos[0] - 1);
    strncpy(is_last_token, data + allDelimiterPos[1]+1, allDelimiterPos[2] - allDelimiterPos[1] - 1);
    strncpy(checksumStr, data + allDelimiterPos[2]+1, allDelimiterPos[3] - allDelimiterPos[2] - 1);
    strncpy(packet_data, data + allDelimiterPos[3]+1, strlen(data) - allDelimiterPos[3] - 1);

    // dynamic memory allocation
    packet = new UDP_PACKET;

    packet->header.sequence_number = atoi(sequence_number);
    packet->header.flag = flag[0];
    packet->header.is_last_packet = is_last_token[0];

    char* endptr;
    packet->header.checksum = strtoul(checksumStr, &endptr, 10);

    strcpy(packet->data, packet_data);

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
        int chunkSize = min(MAX_DATA_SIZE, dataSize - i);

        // set header
        udp_msg->packet.header.sequence_number = packet_sequence_number++;
        udp_msg->packet.header.flag = *flag;
        udp_msg->packet.header.is_last_packet = 'N';

        // copy data
        memcpy(udp_msg->packet.data, data + i, chunkSize);
        udp_msg->packet.data[chunkSize] = '\0';

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
            command_size += strlen(ptr->packet.data);
        }
        else if (ptr->packet.header.flag == *DATA_FLAG) {
            data_size += strlen(ptr->packet.data);
        }
        ptr = ptr->next;
    }

    // reset and begin to copy
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

int sendUDPPacket(int sockfd, UDP_PACKET * packet, const struct sockaddr * remoteAddress) {
    socklen_t serverlen = sizeof(struct sockaddr);

    // create the packet buffer data that is sent over the network
    char * packet_data = new char[MAX_MSG_SIZE];
    packet_data[0] = '\0';

    // serialize the data
    int status = serialize(packet, packet_data);

    if (status != 0) {
        return -1;
    }

    // Send the packet
    cout << "Sending data: " << packet_data << " : " << strlen(packet_data) << endl;
    int bytesSent = sendto(sockfd, packet_data, strlen(packet_data), 0, remoteAddress, serverlen);
    cout << "Bytes sent: " << bytesSent<< endl;

    return bytesSent;
}

int receiveUDPPacket(int sockfd, UDP_PACKET *& packet, struct sockaddr * remoteAddress) {
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

    return bytesReceived;
}


int sendMessage(int sockfd, struct UDP_MSG * message, const struct sockaddr * remoteAddress) {
    vector<int> failedPackets;
    UDP_MSG * p = message;

    //iterate through the message head pointer to send all packets of the list
    while (p != nullptr) {
        // send the packet pointer by p
        int bytesSent = sendUDPPacket(sockfd, &(p->packet), remoteAddress);
        if (bytesSent == -1) {
            failedPackets.push_back(p->packet.header.sequence_number);
        }
        p = p->next;
    }

    // retry failed packets
    cout << "Number of failed packets to send: " << failedPackets.size() << endl;

    return 0;
}

int receiveMessage(int sockfd, struct UDP_MSG *& message_head, struct sockaddr * remoteAddress) {
    UDP_PACKET * packet = nullptr;

    // reset head and tail of the linked message
    message_head = nullptr;
    UDP_MSG * message_tail = nullptr;

    // message is needed for each packet to be encapsulated
    UDP_MSG * message = nullptr;

    int bytesReceived;

    do {
        // receive a packet
        bytesReceived = receiveUDPPacket(sockfd, packet, remoteAddress);

        // put the packet into a new message
        message = new UDP_MSG;
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