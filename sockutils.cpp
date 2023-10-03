#include "sockutils.h"
#include "message.h"

#include <sys/types.h> 
#include <sys/socket.h>

int setSocketTimeout(int sockfd, int timeout) {
    struct timeval tv;
    tv.tv_sec = timeout;
    tv.tv_usec = 0;

    // Set the socket option with a receive timeout set
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == -1) {
        return -1;
    }
    return 0;
}