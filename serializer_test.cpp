#include <iostream>
#include <cerrno>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "message.h"
#include "commands.h"
#include "file_ops.h"
#include "utils.h"

using namespace std;

int main() {
    char *message, *packet_data = nullptr;
    cin >> message;
    // serializer checks

    struct UDP_MSG *udp_message_request = nullptr;

    int last_sequence_number = writeMessage(message, "C", udp_message_request, "N");

    int status = serialize(&(udp_message_request->packet), packet_data);

    cout << packet_data << endl;

    return 0;
}