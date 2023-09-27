#ifndef GBN_H
#define GBN_H

#include "message.h"

#define GBN_VALUE 10
#define RTT 1

const struct UDP_MSG * sendWindow(const struct UDP_MSG *, int, const struct sockaddr *);

void receiveWindow();

void sendAck(int, int, const struct sockaddr *);

int receiveAck(int, int);

#endif