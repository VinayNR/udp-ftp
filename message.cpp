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

    // Data
    strcat(data, packet.data);
}

void deserialize(char data[], struct UDP_PACKET& packet) {
    // Find the position of the delimiter
    char * delimiterPtr = strchr(data, '#');

    if (delimiterPtr != nullptr) {
        // Calculate the length of the number part
        int numberLength = delimiterPtr - data;

        // Extract the number part into a separate character array
        char numberArray[numberLength + 1]; // +1 for the null terminator
        strncpy(numberArray, data, numberLength);
        numberArray[numberLength] = '\0'; // Null-terminate the number array

        // Extract the byte array part
        const char* byteArray = delimiterPtr + 1;

        // Convert the number part to an integer
        packet.header.sequence_number = atoi(numberArray);
        strcpy(packet.data, byteArray);
    }
}

void constructMessage(char *& data, UDP_MSG *& head) {
    UDP_MSG *head = nullptr, *tail = nullptr;
    cout << strlen(data) << " bytes" << endl;
    int dataSize = strlen(data);

    for (int i=0; i<dataSize; i+=MAX_DATA_SIZE) {
        UDP_MSG *msg = new UDP_MSG;
        int chunkSize = min(MAX_DATA_SIZE, dataSize - i);

        // set header
        msg->packet.header.sequence_number = i+1;
        msg->packet.header.is_last_packet = false;

        // copy data
        memcpy(msg->packet.data, data + i, chunkSize);
        msg->packet.data[chunkSize] = '\0';
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
}