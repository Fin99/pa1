#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include "ipc.h"
#include "pa2345.h"
#include "common.h"
#include "banking.h"

void free_memory();

void open_log();

void wait_child_process();

void free_memory_and_wait_child_process();

void read_args(int argc, char const *argv[]);

void alloc_channels();

void create_process();

void open_channel();

void alloc_processes_ids();

void wait_all_message();

Message wait_message_from(int process_id);

void close_log();

void close_other_channels();

Message get_started();

Message get_stop();

Message get_done();

void send_all(Message message);

long *processes_ids;
long child_process;
int **write_channels;
int **read_channels;
int id;

int *balance;

FILE *log_file;

int main(int argc, char const *argv[]) {

    //start program
    read_args(argc, argv);
    alloc_channels();
    alloc_processes_ids();
    open_log();
    open_channel();
    create_process();
    close_other_channels();

    if (id != PARENT_ID) {
        send_all(get_started());
        fprintf(log_file, log_started_fmt, get_physical_time(), id, getpid(), getppid(), balance[id - 1]);
        printf(log_started_fmt, get_physical_time(), id, getpid(), getppid(), balance[id - 1]);
    }
    wait_all_message();
    printf(log_received_all_started_fmt, get_physical_time(), id);
    fprintf(log_file, log_received_all_started_fmt, get_physical_time(), id);
    //////////////// work
    if (id == PARENT_ID) {
        bank_robbery(NULL, child_process);
        printf("send stop\n");
        send_all(get_stop());
    }
    if (id != PARENT_ID) {
        int flag_stop = 1;
        while (flag_stop) {
            Message message = wait_message_from(PARENT_ID);
            if (message.s_header.s_type == STOP) {
                printf("STOP\n");
                flag_stop = 0;
            } else if (message.s_header.s_type == TRANSFER) {
                printf("TRANSFER\n");
            }
        }
    }
    ////////////////
    if (id != PARENT_ID) {
        send_all(get_done());
        fprintf(log_file, log_done_fmt, get_physical_time(), id, balance[id - 1]);
        printf(log_done_fmt, get_physical_time(), id, balance[id - 1]);
    }
    wait_all_message();
    printf(log_received_all_done_fmt, get_physical_time(), id);
    fprintf(log_file, log_received_all_done_fmt, get_physical_time(), id);

    // end program
    close_log();
    free_memory_and_wait_child_process();

    return 0;
}

void send_to(Message message, int destination) {
    send(NULL, destination, &message);
}

void send_all(Message message) {
    send_multicast(NULL, &message);
}

Message get_transfer(int source, int destination, int sum) {
    MessageHeader header;
    header.s_magic = MESSAGE_MAGIC;
    header.s_type = TRANSFER;
    Message message;

    TransferOrder order;
    order.s_src = source;
    order.s_dst = destination;
    order.s_amount = sum;

    message.s_payload = order;

    header.s_payload_len = 0;
    message.s_header = header;
    return message;
}

Message get_stop() {
    MessageHeader header;
    header.s_magic = MESSAGE_MAGIC;
    header.s_type = STOP;
    Message message;
    header.s_payload_len = 0;
    message.s_header = header;
    return message;
}

Message get_done() {
    MessageHeader header;
    header.s_magic = MESSAGE_MAGIC;
    header.s_type = DONE;
    Message message;
    sprintf(message.s_payload, log_done_fmt, get_physical_time(), id, balance[id - 1]);
    header.s_payload_len = strlen(message.s_payload);
    message.s_header = header;
    return message;
}

void transfer(void *parent_data, local_id src, local_id dst, balance_t amount) {
TRANSFER
}

void close_other_channels() {
    for (int source = 0; source < child_process + 1; source++) {
        for (int destination = 0; destination < child_process + 1; destination++) {
            if (source != id && destination != id && source != destination) {
                close(write_channels[source][destination]);
                close(read_channels[source][destination]);
            }
            if (destination == id && source != id) {
                close(write_channels[source][destination]);
            }
            if (source == id && destination != id) {
                close(read_channels[source][destination]);
            }
        }
    }
}

void close_log() {
    fclose(log_file);
}

Message wait_message_from(int process_id) {
    Message message;
    receive(NULL, process_id, &message);
    return message;
}

void wait_all_message() {
    Message message;
    for (int i = 1; i <= child_process; i++) {
        if (i != id) {
            int receive_result = 1;
            while (receive_result) {
                receive_result = receive(NULL, i, &message);
            }
        }
    }
}

void alloc_processes_ids() {
    processes_ids = malloc(sizeof(long) * (child_process + 1));
    processes_ids[PARENT_ID] = getpid();
    id = PARENT_ID;
}

void open_channel() {
    for (int i = 0; i < child_process + 1; i++) {
        for (int j = 0; j < child_process + 1; j++) {
            if (i != j) {
                int pipe_reader_writer[2];
                pipe(pipe_reader_writer);
                read_channels[i][j] = pipe_reader_writer[0];
                fcntl(read_channels[i][j], F_SETFL, O_NONBLOCK);
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
    for (int i = 0; i < child_process + 1; i++) {
        write_channels[i] = malloc(sizeof(long) * (child_process + 1));
        read_channels[i] = malloc(sizeof(long) * (child_process + 1));
    }
}

void read_args(int argc, char const *argv[]) {
    if (argc >= 3 && strcmp(argv[1], "-p") == 0) {
        child_process = strtol(argv[2], NULL, 10);
        if (child_process < 1 || child_process > 10) {
            fprintf(stderr, "Number child process must be in range 1..10\n");
            exit(1);
        }
        if (argc == child_process + 3) {
            balance = malloc(sizeof(int) * child_process);
            for (int i = 0; i < child_process; ++i) {
                balance[i] = strtol(argv[i + 3], NULL, 10);
                if (balance[i] < 1 || balance[i] > 99) {
                    fprintf(stderr, "Child process balance must be in range 1..99\n");
                    exit(1);
                }
            }
        } else {
            fprintf(stderr, "Set child process balance\n");
            exit(1);
        }
    } else {
        fprintf(stderr, "Number child process shouldn't be null\n");
        exit(1);
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

Message get_started() {
    MessageHeader header;
    header.s_magic = MESSAGE_MAGIC;
    header.s_type = STARTED;
    Message message;
    sprintf(message.s_payload, log_started_fmt, get_physical_time(), id, getpid(), getppid(), balance[id - 1]);
    header.s_payload_len = strlen(message.s_payload);
    message.s_header = header;
    return message;
}
