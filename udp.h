#ifndef UDP_H
#define UDP_H

#include "message.h"

int sendMessage(int, UDP_MSG *, const struct sockaddr *);

int sendUDPPacket(int, UDP_PACKET &, const struct sockaddr *);

int receiveUDPPacket(int, UDP_PACKET &, struct sockaddr *);

#endif