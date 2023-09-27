#ifndef MESSAGE_H
#define MESSAGE_H

#include <cstdint>
#include <vector>

extern const int MAX_MSG_SIZE;
extern const int MAX_DATA_SIZE; // adjust this

extern const char * COMMAND_FLAG;
extern const char * DATA_FLAG;

extern const char * APPEND_MSG_MODE;
extern const char * NEW_MSG_MODE;

struct UDP_HEADER {
    uint16_t sequence_number;
    uint32_t checksum;
    char flag; // command or data
    char is_last_packet; // 'Y' or 'N'
};

struct UDP_PACKET {
    UDP_HEADER header; // header
    char data[980]; // the actual data
};

struct UDP_MSG {
    UDP_PACKET packet;
    UDP_MSG * next;
};

// Packet Operations
int serialize(struct UDP_PACKET *, char *);

int deserialize(char *, struct UDP_PACKET *&);

int sendUDPPacket(int, struct UDP_PACKET *, const struct sockaddr *);

int receiveUDPPacket(int, struct UDP_PACKET *&, struct sockaddr *);

// Message Operations
void writeMessage(char *, const char *, struct UDP_MSG *&, const char *);

void readMessage(const struct UDP_MSG *, char *&, char *&);

void deleteMessage(struct UDP_MSG *);

int sendMessage(int, struct UDP_MSG *, const struct sockaddr *);

int receiveMessage(int, struct UDP_MSG *&, struct sockaddr *);

#endif