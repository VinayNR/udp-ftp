#include "message.h"

#include <cstdio>
#include <string>

#include <iostream>

using namespace std;

void serialize(struct UDP_PACKET& packet, char * data) {
    
    // Sequence number serialization
    sprintf(data, "%d", packet.header.sequence_number);

    // Delimiter
    data[strlen(data)] = '#';

    // Last Packet flag
    data[strlen(data)] = packet.header.is_last_packet;

    // Delimiter
    data[strlen(data)] = '#';

    // Checksum
    // Convert the uint32_t checksum to a string
    std::string checksumStr = to_string(packet.header.checksum);

    // Concatenate the checksum string with the data
    strcat(data, checksumStr.c_str());

    // Delimiter
    data[strlen(data)] = '#';

    // Data
    strcat(data, packet.data);
}

void deserialize(char * data, struct UDP_PACKET& packet) {
    // temporary copy
    char sequence_number[8];
    char is_last_token[2];
    char checksumStr[12];
    char packet_data[MAX_DATA_SIZE];

    sscanf(data, "%[^#]#%[^#]#%[^#]#%s", sequence_number, is_last_token, checksumStr, packet_data);

    packet.header.sequence_number = atoi(sequence_number);
    packet.header.is_last_packet = is_last_token[0];

    char* endptr;
    packet.header.checksum = strtoul(checksumStr, &endptr, 10);

    strcpy(packet.data, packet_data);
}

void constructMessage(char *& data, UDP_MSG *& head) {
    UDP_MSG *tail = nullptr;
    cout << strlen(data) << " bytes" << endl;
    int dataSize = strlen(data);

    for (int i=0; i<dataSize; i+=MAX_DATA_SIZE) {
        UDP_MSG *msg = new UDP_MSG;
        int chunkSize = min(MAX_DATA_SIZE, dataSize - i);

        // set header
        msg->packet.header.sequence_number = i+1;
        msg->packet.header.is_last_packet = 'N';

        // copy data
        memcpy(msg->packet.data, data + i, chunkSize);
        msg->packet.data[chunkSize] = '\0';

        // calculate checksum and add to header
        msg->packet.header.checksum = calculateChecksum(msg->packet.data, chunkSize);

        msg->next = nullptr;

        if (head == nullptr) {
            head = msg;
            tail = msg;
        }
        else {
            tail->next = msg;
            tail = msg;
        }
    }
    // set the last packet flag to true
    tail->packet.header.is_last_packet = 'Y';
}

// Function to calculate XOR checksum
uint32_t calculateChecksum(const char* buffer, int length) {
    uint32_t checksum = 0;
    
    for (int i = 0; i < length; i++) {
        checksum ^= buffer[i];
    }
    
    return checksum;
}