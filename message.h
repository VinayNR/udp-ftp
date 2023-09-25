#ifndef MESSAGE_H
#define MESSAGE_H

const int MAX_MSG_SIZE = 1024;
const int MAX_DATA_SIZE = 1000;

const char MSG_DELIMITER = '#';

struct udp_msg {
    int sequence_number; // for ordering and re-transmisions
    char data[MAX_DATA_SIZE]; // the actual data
};

void serialize(struct udp_msg&, char []);

void deserialize(char [], struct udp_msg&);

#endif