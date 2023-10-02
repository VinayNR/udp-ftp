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
    memset(packet_buffer, 0, MAX_MSG_SIZE);

    // Sequence number serialization
    sprintf(packet_buffer, "%d", packet->header.sequence_number);

    cout << "Packet buffer: " << packet_buffer << " : " << strlen(packet_buffer) << endl;

    // Delimiter
    packet_buffer[strlen(packet_buffer)] = '#';

    cout << "Packet buffer: " << packet_buffer << " : " << strlen(packet_buffer) << endl;

    // Command or data flag
    packet_buffer[strlen(packet_buffer)] = packet->header.flag;

    cout << "Packet buffer: " << packet_buffer << " : " << strlen(packet_buffer) << endl;

    // Delimiter
    packet_buffer[strlen(packet_buffer)] = '#';

    cout << "Packet buffer: " << packet_buffer << " : " << strlen(packet_buffer) << endl;

    // Last Packet flag
    packet_buffer[strlen(packet_buffer)] = packet->header.is_last_packet;

    cout << "Packet buffer: " << packet_buffer << " : " << strlen(packet_buffer) << endl;

    // Delimiter
    packet_buffer[strlen(packet_buffer)] = '#';

    cout << "Packet buffer: " << packet_buffer << " : " << strlen(packet_buffer) << endl;

    // Checksum
    // Convert the uint32_t checksum to a string
    string checksumStr = to_string(packet->header.checksum);

    // Concatenate the checksum string with the data
    strcat(packet_buffer, checksumStr.c_str());

    cout << "Packet buffer: " << packet_buffer << " : " << strlen(packet_buffer) << endl;

    // Delimiter
    packet_buffer[strlen(packet_buffer)] = '#';

    cout << "Packet buffer: " << packet_buffer << " : " << strlen(packet_buffer) << endl;

    // Data
    strcat(packet_buffer, packet->data);
    cout << "Packet buffer: " << packet_buffer << " : " << strlen(packet_buffer) << endl;
    // cout << "Copied data in serialization: " << packet_buffer << " : " << strlen(packet_buffer) << endl;

    return 0;
}

int deserialize(const char * packet_buffer, struct UDP_PACKET *& packet) {
    // temporary copy
    char *sequence_number = new char[12];
    memset(sequence_number, 0, 12);
    char flag[2];
    char is_last_token[2];
    char *checksumStr = new char[12];
    memset(checksumStr, 0, 12);
    char * packet_data = new char[MAX_DATA_SIZE];
    memset(packet_data, 0, MAX_DATA_SIZE);

    vector<int> allDelimiterPos = getAllDelimiterPos(packet_buffer, '#');

    // check format
    if (allDelimiterPos.size() != 4) {
        return 1;
    }

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

    // clean up pointers
    deleteAndNullifyPointer(sequence_number, true);
    deleteAndNullifyPointer(checksumStr, true);
    deleteAndNullifyPointer(packet_data, true);

    return 0;
}

int writeMessage(const char *data, const char *flag, struct UDP_MSG *& message_head, const char *msg_write_mode) {
    // cout << "In write message" << endl;
    UDP_MSG *message_tail;
    int packet_sequence_number = 1;
    int dataSize = strlen(data);

    cout << "Size of data in write message : " << dataSize << endl;

    // calculate the tail pointer position
    if (msg_write_mode == NEW_MSG_MODE) {
        message_head = nullptr;
        message_tail = nullptr;
    }
    else if (msg_write_mode == APPEND_MSG_MODE) {
        if (message_head == nullptr) {
            // head cannot be null in append mode
            return -1;
        }
        UDP_MSG *current = message_head;
        message_tail = current;
        while (current->next != nullptr) {
            current = current->next;
            message_tail = current;
        }

        // change is_last_packet of tail to 'N' and get the sequence number
        packet_sequence_number = message_tail->packet.header.sequence_number + 1;
        message_tail->packet.header.is_last_packet = 'N';
    }
    
    UDP_MSG *udp_msg = nullptr;
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

        // calculate checksum and add to header
        udp_msg->packet.header.checksum = djb2_hash(udp_msg->packet.data, chunkSize);

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

    // return the sequence number of the last packet
    return packet_sequence_number - 1;
}

void readMessage(const struct UDP_MSG * message, char *& command, char *& data) {
    cout << "In read message" << endl;
    const struct UDP_MSG * ptr = message;

    int command_size = 0, data_size = 0;

    // calculate total size of the char buffers needed
    while (ptr != nullptr) {
        if (ptr->packet.header.flag == *COMMAND_FLAG) {
            command_size += strlen(ptr->packet.data);
        }
        else if (ptr->packet.header.flag == *DATA_FLAG) {
            data_size += strlen(ptr->packet.data);
            cout << "Data size: " << data_size << endl;
        }
        ptr = ptr->next;
    }

    // reset and begin to copy
    if (command_size > 0) {
        command = new char[command_size + 1];
        memset(command, 0, command_size + 1);
    }
    if (data_size > 0) {
        data = new char[data_size + 1];
        memset(data, 0, data_size + 1);
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
    cout << "In send UDP packet" << endl;
    socklen_t serverlen = sizeof(struct sockaddr);

    // create the packet buffer data that is sent over the network
    char * packet_data = nullptr;

    // serialize the data
    int status = serialize(packet, packet_data);

    if (status != 0) {
        cout << "Serialization failed during send udp packet" << endl;
        return -1;
    }

    // Send the packet
    cout << "Sending data: " << packet_data << " : " << strlen(packet_data) << endl;
    int bytesSent = sendto(sockfd, packet_data, strlen(packet_data), 0, remoteAddress, serverlen);

    if (bytesSent == -1) {
        cerr << "Error sending data: " << strerror(errno) << endl;
    } else {
        cout << "Bytes sent: " << bytesSent << endl;
    }


    // clean up pointers
    deleteAndNullifyPointer(packet_data, true);
    return 0;
}

int receiveUDPPacket(int sockfd, struct UDP_PACKET *& packet, struct sockaddr *remoteAddress) {
    cout << "In receive UDP Packet" << endl;
    socklen_t serverlen = sizeof(struct sockaddr);

    // create the packet buffer data that is filled by the network
    char *packet_data = new char[MAX_MSG_SIZE];
    memset(packet_data, 0, MAX_MSG_SIZE);
    
    // Receive a packet
    int bytesReceived = recvfrom(sockfd, packet_data, MAX_MSG_SIZE, 0, remoteAddress, &serverlen);

    cout << "Received data: " << packet_data << " : " << strlen(packet_data) << endl;
    cout << "Bytes received: " << bytesReceived << endl;

    // deserialize the packet data
    if (deserialize(packet_data, packet) != 0) {
        // failure to deserialize the packet results in an error
        deleteAndNullifyPointer(packet_data, true);
        return -1;
    }

    // clean up pointers
    deleteAndNullifyPointer(packet_data, true);
    return 0;
}

// All send message operations are successful, upon the last ACK from the receiver
int sendMessage(int sockfd, const struct UDP_MSG *message, uint32_t last_sequence_number, const struct sockaddr *remoteAddress) {
    cout << "In send message" << endl;
    const struct UDP_MSG *current_window_start = message, *next_window_start = nullptr;
    int ack_status;
    uint32_t expected_ack_number;

    while (current_window_start != nullptr) {
        // send the window of packets starting from the current_window_start pointer
        next_window_start = sendWindow(current_window_start, sockfd, remoteAddress);

        // if next_window_start is nullptr, then it was the last window of the message
        expected_ack_number = (next_window_start == nullptr)? last_sequence_number: next_window_start->packet.header.sequence_number - 1;
        
        // wait for acknowledgement of expected_ack_number from the receiver
        ack_status = receiveAck(expected_ack_number, sockfd);

        // if the status of acknowledgement is not 0, it is a fail. retransmit the entire window
        if (ack_status != 0) {
            continue;
        }

        // update the current_window_start pointer for the next iteration
        current_window_start = next_window_start;
    }

    return 0;
}

int receiveMessage(int sockfd, struct UDP_MSG *& message_head, struct sockaddr *remoteAddress) {
    cout << "In receive message" << endl;
    // pointers for message and window
    struct UDP_MSG *message_tail = nullptr, *window_start = nullptr, *window_end = nullptr;

    uint32_t expected_sequence_number = 1;

    do {
        // receive a window of packets from the sender identified by start and end message pointers
        receiveWindow(expected_sequence_number, window_start, window_end, sockfd, remoteAddress);
        
        // append the window of packets into the message
        if (message_head == nullptr) {
            message_head = window_start;
            message_tail = window_end;
        }
        else {
            message_tail->next = window_start;
            message_tail = window_end;
        }

        // update next expected sequence number
        expected_sequence_number = message_tail->packet.header.sequence_number + 1;

        // send the acknowledgement back to sender
        sendAck(message_tail->packet.header.sequence_number, sockfd, remoteAddress);

        // reset window pointers
        window_start = nullptr;
        window_end = nullptr;
    } while (message_tail->packet.header.is_last_packet != 'Y');

    return 0;
}