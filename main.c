#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include "ipc.h"
#include "pa1.h"
#include "common.h"

void free_memory();

void open_log();

void wait_child_process();

void free_memory_and_wait_child_process();

void close_channels();

void read_args(int argc, char const *argv[]);

void alloc_channels();

void create_process();

void open_channel();

void alloc_processes_ids();

void send_started();

void wait_message();

void send_done();

void close_log();

long *processes_ids;
long child_process;
int **write_channels;
int **read_channels;
int id;

FILE *log_file;

int main(int argc, char const *argv[]) {

    //start program
    read_args(argc, argv);
    alloc_channels();
    alloc_processes_ids();
    open_log();
    open_channel();
    create_process();

    if (id != PARENT_ID) {
        send_started();
    }
    wait_message();
    printf(log_received_all_started_fmt, id);
    fprintf(log_file, log_received_all_started_fmt, id);

    if (id != PARENT_ID) {
        send_done();
    }
    wait_message();
    printf(log_received_all_done_fmt, id);
    fprintf(log_file, log_received_all_done_fmt, id);

    // end program
    close_log();
    close_channels();
    free_memory_and_wait_child_process();

    return 0;
}

void close_log() {
    fclose(log_file);
}

void send_done() {
    MessageHeader header;
    header.s_magic = MESSAGE_MAGIC;
    header.s_type = DONE;
    Message message;
    sprintf(message.s_payload, log_done_fmt, id);
    header.s_payload_len = strlen(message.s_payload);
    message.s_header = header;
    send_multicast(NULL, &message);
    fprintf(log_file, log_done_fmt, id);
    printf(log_done_fmt, id);
}

void wait_message() {
    Message message;
    for (int i = 1; i <= child_process; i++) {
        if (i != id) {
            receive(NULL, i, &message);
        }
    }
}

void send_started() {
    MessageHeader header;
    header.s_magic = MESSAGE_MAGIC;
    header.s_type = STARTED;
    Message message;
    sprintf(message.s_payload, log_started_fmt, id, getpid(), getppid());
    header.s_payload_len = strlen(message.s_payload);
    message.s_header = header;
    send_multicast(NULL, &message);
    fprintf(log_file, log_started_fmt, id, getpid(), getppid());
    printf(log_started_fmt, id, getpid(), getppid());
}

void alloc_processes_ids() {
    processes_ids = malloc(sizeof(long) * (child_process + 1));
    processes_ids[PARENT_ID] = getpid();
    id = PARENT_ID;
}

void open_channel() {
    for (int i = 0; i < child_process + 1; i++) {
        for (int j = 0; j < child_process + 1; j++) {
            if (i != id) {
                int pipe_reader_writer[2];
                pipe(pipe_reader_writer);
                read_channels[i][j] = pipe_reader_writer[0];
                write_channels[i][j] = pipe_reader_writer[1];
            }
        }
    }
}

void create_process() {
    for (int i = 1; i <= child_process; i++) {
        int fork_result = fork();
        if (fork_result == 0) {
            id = i;
            break;
        } else if (fork_result < 0) {
            fprintf(stderr, "Fork error\n");
        } else {
            processes_ids[i] = fork_result;
        }
    }
}

void alloc_channels() {
    write_channels = malloc(sizeof(long) * (child_process + 1));
    read_channels = malloc(sizeof(long) * (child_process + 1));
    for (int i = 0; i <= child_process; ++i) {
        write_channels[i] = malloc(sizeof(long) * (child_process + 1));
        read_channels[i] = malloc(sizeof(long) * (child_process + 1));
    }
}

void read_args(int argc, char const *argv[]) {
    if (argc == 3 && strcmp(argv[1], "-p") == 0) {
        child_process = strtol(argv[2], NULL, 10);
        if (child_process < 1 || child_process > 10) {
            fprintf(stderr, "Number child process must be in range 1..10\n");
            exit(1);
        }
    } else {
        fprintf(stderr, "Number child process shouldn't be null\n");
        exit(1);
    }
}

void close_channels() {
    if (id == PARENT_ID) {
        for (int i = 0; i < child_process + 1; i++) {
            for (int j = 0; j < child_process + 1; ++j) {
                if (i != id) {
                    close(read_channels[i][j]);
                    close(write_channels[i][j]);
                }
            }
        }
    }
}

void free_memory_and_wait_child_process() {
    if (id != PARENT_ID) {
        free_memory();
    }

    if (id == PARENT_ID) {
        wait_child_process();
        free_memory();
    }
}

void wait_child_process() {
    for (int i = 1; i <= child_process; i++) {
        waitpid(processes_ids[i], NULL, 0);
    }
}

void free_memory() {
    for (int i = 0; i <= child_process; i++) {
        free(read_channels[i]);
        free(write_channels[i]);
    }
    free(processes_ids);
    free(read_channels);
    free(write_channels);
}


void open_log() {
    log_file = fopen(events_log, "w");
}
