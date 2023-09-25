#include "message.h"

#include <cstdio>
#include <string>

#include <iostream>

using namespace std;

void serialize(struct udp_msg& message, char data[]) {
    // Sequence number serialization
    sprintf(data, "%d", message.sequence_number);
    // Delimiter
    strcat(data, &MSG_DELIMITER);
    // Data
    strcat(data, message.data);
    // End of packet data
    data[strlen(data)] = '\0';
}

void deserialize(char data[], struct udp_msg& message) {
    // Find the position of the delimiter
    char * delimiterPtr = strchr(data, MSG_DELIMITER);

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
        message.sequence_number = atoi(numberArray);
        strcpy(message.data, byteArray);
    }
}