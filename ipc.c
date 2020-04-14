#include <unistd.h>
#include <stdio.h>
#include "ipc.h"

long child_process;
int **write_channels;
int **read_channels;
int id;

int send(void *data, local_id destination, const Message *message) {
    const MessageHeader *header = &message->s_header;
    printf("Send message to %i from %i, message_type %i\n", id, destination, header->s_type);
    fflush(stdout);
    write(write_channels[id][destination], header, sizeof(MessageHeader));
    write(write_channels[id][destination], &message->s_payload, message->s_header.s_payload_len);
    printf("Ready message to %i from %i, message_type %i\n", id, destination, header->s_type);
    fflush(stdout);
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
    int suka = read(read_channels[from][id], &message->s_header, sizeof(MessageHeader));
    if (suka > 0) {
        int read_byte;
        int remained_byte;
        int header_readed;

        if (suka == sizeof(MessageHeader)){
            header_readed = 1;
            read_byte = 0;
            remained_byte = message->s_header.s_payload_len;

            printf("!%i header. Message type %i\n", id, message->s_header.s_type);
            fflush(stdout);
            if (remained_byte == 0) {
                printf("!%i body. Empty\n", id);
                fflush(stdout);
                return 0;
            }
        } else {
            header_readed = 0;
            read_byte = suka;
            remained_byte = sizeof(MessageHeader);
            remained_byte -= suka;

            printf("!%i. Read bytes %i\n", id, suka);
            fflush(stdout);
        }
        while (1) {
            if (!header_readed) {
                int new_byte = read(read_channels[from][id], ((char *)&message->s_header) + read_byte, remained_byte);
                if (new_byte > 0) {
                    read_byte += new_byte;
                    remained_byte -= new_byte;
                    if (remained_byte == 0) {
                        header_readed = 1;
                        read_byte = 0;
                        remained_byte = message->s_header.s_payload_len;

                        printf("!%i header. Message type %i\n", id, message->s_header.s_type);
                        fflush(stdout);

                        if (remained_byte == 0) {
                            printf("!%i body. Empty\n", id);
                            fflush(stdout);
                            return 0;
                        }
                    } else {
                        printf("!%i. Read bytes %i\n", id, read_byte);
                        fflush(stdout);
                    }
                }
            }
            if (header_readed) {
                int new_byte = read(read_channels[from][id], ((char *)&message->s_payload) + read_byte, remained_byte);
                if (new_byte > 0) {
                    read_byte += new_byte;
                    remained_byte -= new_byte;
                    if (remained_byte == 0) {
                        printf("!%i body\n", id);
                        fflush(stdout);
                        return 0;
                    } else {
                        printf("!%i. Read bytes %i\n", id, read_byte);
                        fflush(stdout);
                    }
                }
            }
        }
    }
    return 1;
}


