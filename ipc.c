#include <unistd.h>
#include "ipc.h"

long child_process;
int **write_channels;
int **read_channels;
int id;

int send(void *data, local_id destination, const Message *message) {
    const MessageHeader *header = &message->s_header;
    write(write_channels[id][destination], header, sizeof(MessageHeader));
    write(write_channels[id][destination], &message->s_payload, message->s_header.s_payload_len);
    return 0;
}

int send_multicast(void *self, const Message *message) {
    for (int destination_process = 0; destination_process <= child_process; destination_process++) {
        if (destination_process != id) {
            send(self, destination_process, message);
        }
    }
    return 0;
}

int receive(void *self, local_id from, Message *message) {
    read(read_channels[from][id], &message->s_header, sizeof(MessageHeader));
    read(read_channels[from][id], &message->s_payload, message->s_header.s_payload_len);
    return 0;
}


