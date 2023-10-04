#ifndef GBN_H
#define GBN_H

#include "message.h"

#define GBN_VALUE 10
#define RTT 2

const struct UDP_MSG * sendWindow(const struct UDP_MSG *, int, const struct sockaddr *);

void receiveWindow(uint32_t, struct UDP_MSG *&, struct UDP_MSG *&, int, struct sockaddr *);

void sendAck(uint32_t, int, const struct sockaddr *);

int receiveAck(uint32_t, int);

void sendException(uint32_t, int, const struct sockaddr *);

#endif