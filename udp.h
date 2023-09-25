#ifndef UDP_H
#define UDP_H

#include "message.h"

int sendMessage(int sockfd, UDP_MSG * message, const struct sockaddr * remoteAddress);

int sendUDPPacket(int sockfd, UDP_PACKET * packet, const struct sockaddr * remoteAddress);

int receiveUDPPacket(int sockfd, UDP_PACKET * packet, struct sockaddr * remoteAddress);

#endif