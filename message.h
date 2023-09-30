#ifndef MESSAGE_H
#define MESSAGE_H

#include <cstdint>
#include <vector>

extern const int MAX_MSG_SIZE;
extern const int MAX_DATA_SIZE; // adjust this

extern const char * COMMAND_FLAG;
extern const char * DATA_FLAG;
extern const char * ACK_FLAG;

extern const char * APPEND_MSG_MODE;
extern const char * NEW_MSG_MODE;

struct UDP_HEADER {
    uint32_t sequence_number;
    uint32_t checksum;
    char flag; // command or data or ack
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
int serialize(const struct UDP_PACKET*, char *&);

int deserialize(const char*, struct UDP_PACKET *&);

int sendUDPPacket(int, const struct UDP_PACKET*, const struct sockaddr*);

int receiveUDPPacket(int, struct UDP_PACKET *&, struct sockaddr*);

// Message Operations
int writeMessage(const char*, const char*, struct UDP_MSG *&, const char*);

void readMessage(const struct UDP_MSG*, char *&, char *&);

void deleteMessage(struct UDP_MSG*);

int sendMessage(int, const struct UDP_MSG*, uint32_t, const struct sockaddr*);

int receiveMessage(int, struct UDP_MSG *&, struct sockaddr*);

#endif