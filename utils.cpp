#include "utils.h"

#include <vector>

using namespace std;

// Function to calculate XOR checksum
uint32_t calculateChecksum(const char* buffer, int length) {
    uint32_t checksum = 0;
    
    for (int i = 0; i < length; i++) {
        checksum ^= buffer[i];
    }
    
    return checksum;
}

uint32_t djb2_hash(const char* buffer, int length) {
    uint32_t hash = 5381; // Initial hash value

    for (int i = 0; i < length; i++) {
        hash = ((hash << 5) + hash) + buffer[i];
    }
    
    return hash;
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