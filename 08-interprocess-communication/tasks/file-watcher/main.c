#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/reg.h>
#include <syscall.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdint.h>

enum { MAX_PATH_SIZE = 64 };

typedef struct Counter {
    char            filename[PATH_MAX];
    int             count;
    struct Counter* next;
} Counter;

typedef struct Counters {
    Counter* head;
} Counters;

static void increment_counter(Counters* counters, const char* filename, int increment) {
    for (Counter* cur = counters->head; cur != NULL; cur = cur->next) {
        if (strncmp(cur->filename, filename, PATH_MAX) == 0) {
            cur->count += increment;
            return;
        }
    }

    Counter* new_item = (Counter*)malloc(sizeof(Counter));
    if (!new_item) {
        fprintf(stderr, "Failed to allocate memory for counter\n");
        exit(EXIT_FAILURE);
    }

    new_item->count = increment;
    strncpy(new_item->filename, filename, PATH_MAX - 1);
    new_item->filename[PATH_MAX - 1] = '\0';
    new_item->next = counters->head;
    counters->head = new_item;
}

static void print_counters(const Counters* counters) {
    for (Counter* cur = counters->head; cur != NULL; cur = cur->next) {
        printf("%s:%d\n", cur->filename, cur->count);
    }
}

static void resolve_fd_path(pid_t pid, int fd, char* buffer) {
    char    proc_fd_path[MAX_PATH_SIZE];
    ssize_t path_len;

    snprintf(proc_fd_path, sizeof(proc_fd_path), "/proc/%d/fd/%d", pid, fd);
    path_len = readlink(proc_fd_path, buffer, MAX_PATH_SIZE - 1);

    if (path_len >= 0) {
        buffer[path_len] = '\0';
    } else {
        buffer[0] = '\0';
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <program> [args...]\n", argv[0]);
        return EXIT_FAILURE;
    }

    Counters counters_storage;
    counters_storage.head = NULL;

    pid_t target_pid = fork();
    if (target_pid < 0) {
        perror("fork");
        return EXIT_FAILURE;
    }

    if (target_pid == 0) {
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        execvp(argv[1], &argv[1]);
        perror("execvp");
        exit(EXIT_FAILURE);
    }

    int       status;
    uint64_t  syscall_num;
    int       fd_val        = 0;
    int       written_bytes = 0;
    int       in_syscall    = 0;
    char      path_buf[MAX_PATH_SIZE];

    while (1) {
        wait(&status);
        if (WIFEXITED(status)) {
            break;
        }

        syscall_num = ptrace(PTRACE_PEEKUSER, target_pid, sizeof(long) * ORIG_RAX, NULL);

        if (syscall_num == SYS_write) {
            if (!in_syscall) {
                in_syscall = 1;
                fd_val = ptrace(PTRACE_PEEKUSER, target_pid, sizeof(long) * RDI, NULL);
            } else {
                written_bytes = ptrace(PTRACE_PEEKUSER, target_pid, sizeof(long) * RAX, NULL);
                in_syscall    = 0;

                resolve_fd_path(target_pid, fd_val, path_buf);
                if (path_buf[0] != '\0') {
                    increment_counter(&counters_storage, path_buf, written_bytes);
                }
            }
        }

        ptrace(PTRACE_SYSCALL, target_pid, NULL, NULL);
    }

    print_counters(&counters_storage);

    return 0;
}
