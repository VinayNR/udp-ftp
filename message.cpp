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

const char * COMMAND_FLAG = "C";
const char * DATA_FLAG = "D";
const char * ACK_FLAG = "A";
const char * EXCP_FLAG = "E";

const char * APPEND_MSG_MODE = "A";
const char * NEW_MSG_MODE = "N";

/*
Serialization Format
<sequence_number>#<flag>#<is_last_packet>#<checksum>#<data>
*/
int serialize(const struct UDP_PACKET * packet, char *& packet_buffer) {
    
    // size of the packet buffer after serialization
    int total_size = 0;

    // allocate memory for the packet_buffer
    packet_buffer = new char[MAX_MSG_SIZE];
    memset(packet_buffer, 0, MAX_MSG_SIZE);

    // Sequence number serialization
    sprintf(packet_buffer, "%d", packet->header.sequence_number);

    total_size += strlen(packet_buffer);

    // cout << "Packet buffer: " << packet_buffer << " : " << strlen(packet_buffer) << endl;

    // Delimiter
    packet_buffer[strlen(packet_buffer)] = PACKET_DELIMITER;

    total_size += 1;

    // cout << "Packet buffer: " << packet_buffer << " : " << strlen(packet_buffer) << endl;

    // Command or data flag
    packet_buffer[strlen(packet_buffer)] = packet->header.flag;
    total_size += 1;

    // cout << "Packet buffer: " << packet_buffer << " : " << strlen(packet_buffer) << endl;

    // Delimiter
    packet_buffer[strlen(packet_buffer)] = PACKET_DELIMITER;
    total_size += 1;

    // cout << "Packet buffer: " << packet_buffer << " : " << strlen(packet_buffer) << endl;

    // Last Packet flag
    packet_buffer[strlen(packet_buffer)] = packet->header.is_last_packet;
    total_size += 1;

    // cout << "Packet buffer: " << packet_buffer << " : " << strlen(packet_buffer) << endl;

    // Delimiter
    packet_buffer[strlen(packet_buffer)] = PACKET_DELIMITER;
    total_size += 1;

    // cout << "Packet buffer: " << packet_buffer << " : " << strlen(packet_buffer) << endl;

    // Checksum
    // Convert the uint32_t checksum to a string
    string checksumStr = to_string(packet->header.checksum);

    // Concatenate the checksum string with the data
    strcat(packet_buffer, checksumStr.c_str());
    total_size += checksumStr.length();

    // cout << "Packet buffer: " << packet_buffer << " : " << strlen(packet_buffer) << endl;

    // Delimiter
    packet_buffer[strlen(packet_buffer)] = PACKET_DELIMITER;
    total_size += 1;

    // cout << "Packet buffer: " << packet_buffer << " : " << strlen(packet_buffer) << endl;

    // Data
    memcpy(packet_buffer + strlen(packet_buffer), packet->data, packet->header.dataSize);

    // update total size
    total_size += packet->header.dataSize;

    packet_buffer[total_size] = '\0';
    // strcat(packet_buffer, packet->data);
    // cout << "Packet buffer: " << packet_buffer << " : " << strlen(packet_buffer) << endl;

    return total_size;
}

int deserialize(const char * packet_buffer, int packetSize, struct UDP_PACKET *& packet) {
    // temporary copy
    char *sequence_number = new char[12];
    memset(sequence_number, 0, 12);
    char flag[2];
    char is_last_token[2];
    char *checksumStr = new char[12];
    memset(checksumStr, 0, 12);
    char * packet_data = new char[MAX_DATA_SIZE];
    memset(packet_data, 0, MAX_DATA_SIZE);

    vector<int> allDelimiterPos = getAllDelimiterPos(packet_buffer, PACKET_DELIMITER);

    // check format
    if (allDelimiterPos.size() < 4) {
        cout << "Number of delims is not 4" << endl;
        return -1;
    }

    strncpy(sequence_number, packet_buffer, allDelimiterPos[0]);
    strncpy(flag, packet_buffer + allDelimiterPos[0]+1, allDelimiterPos[1] - allDelimiterPos[0] - 1);
    strncpy(is_last_token, packet_buffer + allDelimiterPos[1]+1, allDelimiterPos[2] - allDelimiterPos[1] - 1);
    strncpy(checksumStr, packet_buffer + allDelimiterPos[2]+1, allDelimiterPos[3] - allDelimiterPos[2] - 1);

    // actual size of data sent (excludes the part of header)
    int dataSize = packetSize - allDelimiterPos[3] - 1;
    memcpy(packet_data, packet_buffer + allDelimiterPos[3]+1, dataSize);

    // null terminate it
    packet_data[dataSize] = '\0';

    // dynamic memory allocation
    packet = new UDP_PACKET;
    memset(packet, 0, sizeof(UDP_PACKET));

    packet->header.sequence_number = atoi(sequence_number);
    packet->header.flag = flag[0];
    packet->header.is_last_packet = is_last_token[0];

    char* endptr;
    packet->header.checksum = strtoul(checksumStr, &endptr, 10);

    memcpy(packet->data, packet_data, dataSize);
    packet->data[dataSize] = '\0';

    // set the size also in the message structure
    packet->header.dataSize = dataSize;

    // clean up pointers
    deleteAndNullifyPointer(sequence_number, true);
    deleteAndNullifyPointer(checksumStr, true);
    deleteAndNullifyPointer(packet_data, true);

    return 0;
}

int writeMessage(const char *data, int dataSize, const char *flag, struct UDP_MSG *& message_head, const char *msg_write_mode) {
    // cout << "In write message" << endl;
    UDP_MSG *message_tail;
    int packet_sequence_number = 1;

    // cout << "Size of data in write message : " << dataSize << endl;

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
        memcpy(udp_msg->packet.data, data+i, chunkSize);
        udp_msg->packet.data[chunkSize] = '\0';

        // set the packet size -> useful while sending the message
        udp_msg->packet.header.dataSize = chunkSize;

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

    // display it
    // displayMessage(message_head);

    // return the sequence number of the last packet
    return packet_sequence_number - 1;
}

void displayMessage(struct UDP_MSG *& message_head) {
    const UDP_MSG * current = message_head;
    while (current != nullptr) {
        cout << "Size of data: " << strlen(current->packet.data) << endl;
        current = current->next; 
    }
}

void readMessage(const struct UDP_MSG * message, char *& command, int *serverResponseSize, char *& data, int *serverDataSize) {
    // cout << "In read message" << endl;
    const struct UDP_MSG * ptr = message;

    int command_size = 0, data_size = 0;
    *serverResponseSize = 0;
    *serverDataSize = 0;

    // calculate total size of the char buffers needed
    while (ptr != nullptr) {
        if (ptr->packet.header.flag == *COMMAND_FLAG) {
            command_size += ptr->packet.header.dataSize;
        }
        else if (ptr->packet.header.flag == *DATA_FLAG) {
            data_size += ptr->packet.header.dataSize;
        }
        ptr = ptr->next;
    }

    // reset and begin to copy
    if (command_size > 0) {
        command = new char[command_size + 1];
        *serverResponseSize = command_size;
        memset(command, 0, command_size + 1);
    }
    if (data_size > 0) {
        data = new char[data_size + 1];
        *serverDataSize = data_size;
        memset(data, 0, data_size + 1);
    }
    
    // reset pointer to head of message
    ptr = message;

    int memcpy_idx = 0;
    while (ptr != nullptr) {
        if (ptr->packet.header.flag == *COMMAND_FLAG) {
            strcat(command, ptr->packet.data);
        }
        else if (ptr->packet.header.flag == *DATA_FLAG) {
            // use memcpy for any data transfer (could include binary)
            memcpy(data+memcpy_idx, ptr->packet.data, ptr->packet.header.dataSize);
            memcpy_idx += ptr->packet.header.dataSize;
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
    int packetSize = serialize(packet, packet_data);

    if (packetSize == 0) {
        cout << "Serialization failed during send udp packet" << endl;
        return -1;
    }

    // Send the packet
    // cout << "Sending data: " << packet_data << " : " << packetSize << endl;
    int bytesSent = sendto(sockfd, packet_data, packetSize, 0, remoteAddress, serverlen);

    if (bytesSent == -1) {
        cerr << "Error sending data: " << strerror(errno) << endl;
    } else {
        // cout << "Bytes sent: " << bytesSent << endl;
    }

    // clean up pointers
    deleteAndNullifyPointer(packet_data, true);
    return bytesSent;
}

int receiveUDPPacket(int sockfd, struct UDP_PACKET *& packet, struct sockaddr *remoteAddress) {
    cout << "In receive UDP Packet" << endl;
    socklen_t serverlen = sizeof(struct sockaddr);

    // create the packet buffer data that is filled by the network
    char *packet_data = new char[MAX_MSG_SIZE];
    memset(packet_data, 0, MAX_MSG_SIZE);
    
    // Receive a packet
    int bytesReceived = recvfrom(sockfd, packet_data, MAX_MSG_SIZE, 0, remoteAddress, &serverlen);

    // cout << "Received data: " << packet_data << " : " << bytesReceived << endl;
    // cout << "Bytes received: " << bytesReceived << endl;

    if (bytesReceived == -1)  {
        return -1;
    }
    // deserialize the packet data
    if (deserialize(packet_data, bytesReceived, packet) == -1) {
        // failure to deserialize the packet results in an error
        cout << "Failed to deserialize" << endl;
        deleteAndNullifyPointer(packet_data, true);
        return -1;
    }

    // clean up pointers
    deleteAndNullifyPointer(packet_data, true);
    return bytesReceived;
}

// All send message operations are successful, upon the last ACK from the receiver
int sendMessage(int sockfd, const struct UDP_MSG *message, uint32_t last_sequence_number, const struct sockaddr *remoteAddress) {
    cout << endl << "----------------------------------------" << endl;
    cout << "In send message" << endl;
    const struct UDP_MSG *current_window_start = message, *next_window_start = nullptr;
    int ack_status;
    uint32_t expected_ack_number;

    while (current_window_start != nullptr) {
        // send the window of packets starting from the current_window_start pointer
        next_window_start = sendWindow(current_window_start, sockfd, remoteAddress);

        // if next_window_start is nullptr, then it was the last window of the message
        expected_ack_number = (next_window_start == nullptr)? last_sequence_number: next_window_start->packet.header.sequence_number - 1;
        
        // wait for acknowledgement of expected_ack_number from the receiver, or an excpetion
        try {
            ack_status = receiveAck(expected_ack_number, sockfd);
            // if the status of acknowledgement is -1, it is a fail, retransmit the entire window
            if (ack_status == -1) {
                continue;
            }
        } catch(uint32_t excp_number) {
            if (excp_number == next_window_start->packet.header.sequence_number) {
                // update the current_window_start pointer for the next iteration
                current_window_start = next_window_start;
            }
        }
        
        // update the current_window_start pointer for the next iteration
        current_window_start = next_window_start;
    }

    return 0;
}

int receiveMessage(int sockfd, struct UDP_MSG *& message_head, struct sockaddr *remoteAddress) {
    cout << endl << "----------------------------------------" << endl;
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

struct UDP_PACKET * constructSimplePacket(uint32_t sequence_number, char flag_type, char *data) {
    struct UDP_PACKET *simplePacket = new UDP_PACKET;
    memset(simplePacket, 0, sizeof(UDP_PACKET));

    simplePacket->header.sequence_number = sequence_number;
    simplePacket->header.flag = flag_type;
    simplePacket->header.is_last_packet = 'Y';

    strcpy(simplePacket->data, data);
    simplePacket->header.dataSize = strlen(simplePacket->data);
    simplePacket->header.checksum = djb2_hash(simplePacket->data, strlen(simplePacket->data));

    return simplePacket;
}