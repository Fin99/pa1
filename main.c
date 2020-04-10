#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include "ipc.h"

void free_memory();

void wait_child_process();

void free_memory_and_wait_child_process();

void close_channels();

void read_args(int argc, char const *argv[]);

void alloc_channels();

void create_process();

void open_channel();

long *processes_ids;
long child_process;
int **write_channels;
int **read_channels;
long id;

int main(int argc, char const *argv[]) {

    read_args(argc, argv);

    alloc_channels();

    processes_ids = malloc(sizeof(long) * (child_process + 1));
    processes_ids[PARENT_ID] = getpid();
    id = PARENT_ID;

    create_process();

    open_channel();

    if (id != processes_ids[PARENT_ID])
        printf("Process created pid=%d, ppid=%d\n", getpid(), getppid());

    close_channels();

    free_memory_and_wait_child_process();

    return 0;
}

void open_channel() {
    for (int i = 0; i < child_process + 1; i++) {
        if (i != id) {
            int pipe_reader_writer[2];
            pipe(pipe_reader_writer);
            read_channels[id][i] = pipe_reader_writer[0];
            write_channels[id][i] = pipe_reader_writer[1];
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
    for (int i = 0; i < child_process + 1; i++) {
        if (i != id) {
            close(read_channels[id][i]);
            close(write_channels[id][i]);
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
