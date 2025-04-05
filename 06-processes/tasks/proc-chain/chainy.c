#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

enum {
    MAX_ARGS_COUNT        = 256,
    MAX_CHAIN_LINKS_COUNT = 256
};

typedef struct {
    char    *cmd;
    uint64_t argc;
    char    *argv[MAX_ARGS_COUNT];
} command_t;

typedef struct {
    uint64_t count;
    command_t links[MAX_CHAIN_LINKS_COUNT];
} pipeline_t;

static void parse_pipeline(char *input, pipeline_t *pipeline) {
    pipeline->count = 0;
    for (char *part = strtok(input, "|"); part; part = strtok(NULL, "|")) {
        if (pipeline->count >= MAX_CHAIN_LINKS_COUNT) {
            fprintf(stderr, "Слишком много команд в конвейере\n");
            exit(EXIT_FAILURE);
        }
        command_t *cmd = &pipeline->links[pipeline->count++];
        cmd->cmd = part;
    }

    for (size_t i = 0; i < pipeline->count; ++i) {
        command_t *cmd = &pipeline->links[i];
        cmd->argc = 0;
        for (char *arg = strtok(cmd->cmd, " "); arg; arg = strtok(NULL, " ")) {
            if (*arg) {
                if (cmd->argc >= MAX_ARGS_COUNT - 1) {
                    fprintf(stderr, "Слишком много аргументов в команде\n");
                    exit(EXIT_FAILURE);
                }
                cmd->argv[cmd->argc++] = arg;
            }
        }
        cmd->argv[cmd->argc] = NULL;
        if (cmd->argc > 0) {
            cmd->cmd = cmd->argv[0];
        } else {
            cmd->cmd = NULL;
        }
    }
}

static void execute_pipeline(pipeline_t *pipeline) {
    if (pipeline->count == 0) {
        return;
    }
    int original_stdout = dup(STDOUT_FILENO);
    if (original_stdout == -1) {
        perror("dup");
        exit(EXIT_FAILURE);
    }
    for (size_t i = 0; i < pipeline->count; ++i) {
        int pipe_fds[2];
        if (i < pipeline->count - 1) {
            if (pipe(pipe_fds) == -1) {
                perror("pipe");
                exit(EXIT_FAILURE);
            }
            if (dup2(pipe_fds[1], STDOUT_FILENO) == -1) {
                perror("dup2");
                exit(EXIT_FAILURE);
            }
            close(pipe_fds[1]);
        } else {
            if (dup2(original_stdout, STDOUT_FILENO) == -1) {
                perror("dup2");
                exit(EXIT_FAILURE);
            }
            close(original_stdout);
        }
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        if (pid == 0) {
            if (i < pipeline->count - 1) {
                close(pipe_fds[0]);
            }
            execvp(pipeline->links[i].cmd, pipeline->links[i].argv);
            perror("execvp");
            exit(EXIT_FAILURE);
        }
        if (i < pipeline->count - 1) {
            if (dup2(pipe_fds[0], STDIN_FILENO) == -1) {
                perror("dup2");
                exit(EXIT_FAILURE);
            }
            close(pipe_fds[0]);
        }
    }
    int status = 0;
    while (wait(&status) > 0)
        ;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Использование: %s \"команда1 | команда2 | ...\"\n", argv[0]);
        return EXIT_FAILURE;
    }
    pipeline_t pipeline;
    parse_pipeline(argv[1], &pipeline);
    execute_pipeline(&pipeline);
    return EXIT_SUCCESS;
}
