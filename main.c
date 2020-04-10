#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char const *argv[]) {
    long child_process;

    // read args (number process)
    if (argc == 3 && strcmp(argv[1], "-p") == 0) {
        child_process = strtol(argv[2], NULL, 10);
        if (child_process < 1 || child_process > 10) {
            fprintf(stderr, "Number child process must be in range 1..10\n");
            return 1;
        }
    } else {
        fprintf(stderr, "Number child process shouldn't be null\n");
        return 1;
    }

    for (int id = 1; id <= child_process; id++) {
        int fork_result = fork();
        if (fork_result == 0) {
            break;
        }
    }

    printf("Process created pid=%d, ppid=%d\n", getpid(), getppid());

    return 0;
}
