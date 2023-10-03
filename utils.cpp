#include "utils.h"

#include <vector>
#include <iostream>

using namespace std;

uint32_t djb2_hash(const char* buffer, int length) {
    uint32_t hash = 5381; // Initial hash value

    for (int i = 0; i < length; i++) {
        hash = ((hash << 5) + hash) + buffer[i];
    }
    
    return hash;
}

bool validateChecksum(const char * packet_data, int length, uint32_t checksum) {
    // compute the checksum of the packet
    uint32_t computedChecksum = djb2_hash(packet_data, length);
    cout << "Computed checksum: " << computedChecksum << " and obtained checksum: " << checksum << endl;
    if (computedChecksum != checksum) {
        // discard the packet
        cout << "Discarded packet because checksum failed" << endl;
        return false;
    }
    return true;
}

vector<int> getAllDelimiterPos(const char * data, char delimiter) {
    vector<int> positions;

    for (int i = 0; data[i] != '\0'; i++) {
        if (data[i] == delimiter) {
            positions.push_back(i);
        }
    }

    return positions;
}

// template <typename T>
void deleteAndNullifyPointer(char *& ptr, bool isArray) {
    isArray ? delete[] ptr : delete ptr;
    ptr = nullptr;
}