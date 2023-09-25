#ifndef MESSAGE_H
#define MESSAGE_H

#include <cstdint>

const int MAX_MSG_SIZE = 1024;
const int MAX_DATA_SIZE = 1000;

struct UDP_HEADER {
    uint16_t sequence_number;
    uint32_t checksum;
    bool is_last_packet;
};

struct UDP_PACKET {
    UDP_HEADER header; // header
    char data[MAX_DATA_SIZE]; // the actual data
};

struct UDP_MSG {
    UDP_PACKET packet; // header
    UDP_MSG * next;
};

void serialize(struct UDP_PACKET&, char *);

void deserialize(char [], struct UDP_PACKET&);

void constructMessage(char *&, UDP_MSG *&);

#endif