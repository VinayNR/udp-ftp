#ifndef MESSAGE_H
#define MESSAGE_H

#include <cstdint>
#include <vector>

#define MAX_MSG_SIZE 1024
#define MAX_DATA_SIZE 980

#define PACKET_DELIMITER '#'

extern const char * COMMAND_FLAG;
extern const char * DATA_FLAG;
extern const char * ACK_FLAG;
extern const char * EXCP_FLAG;

extern const char * APPEND_MSG_MODE;
extern const char * NEW_MSG_MODE;

struct UDP_HEADER {
    uint32_t sequence_number;
    uint32_t checksum;
    char flag; // command or data or ack
    char is_last_packet; // 'Y' or 'N'
    int dataSize; // useful for memcpy
};

struct UDP_PACKET {
    UDP_HEADER header; // header
    char data[MAX_DATA_SIZE + 1]; // the actual data
};

struct UDP_MSG {
    UDP_PACKET packet;
    UDP_MSG * next;
};

// Packet Operations
int serialize(const struct UDP_PACKET*, char *&);

int deserialize(const char*, int, struct UDP_PACKET *&);

int sendUDPPacket(int, const struct UDP_PACKET*, const struct sockaddr*);

int receiveUDPPacket(int, struct UDP_PACKET *&, struct sockaddr*);

struct UDP_PACKET * constructSimplePacket(uint32_t, char, char *);

// Message Operations
int writeMessage(const char*, int, const char*, struct UDP_MSG *&, const char*);

void readMessage(const struct UDP_MSG*, char *&, int *, char *&, int *);

void deleteMessage(struct UDP_MSG*);

int sendMessage(int, const struct UDP_MSG*, uint32_t, const struct sockaddr*);

int receiveMessage(int, struct UDP_MSG *&, struct sockaddr*);

void displayMessage(struct UDP_MSG *&);

#endif