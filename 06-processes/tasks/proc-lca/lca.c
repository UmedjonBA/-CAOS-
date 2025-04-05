#include "lca.h"

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

pid_t get_parent_pid(const pid_t pid) {
    char stat_path[32];
    snprintf(stat_path, sizeof(stat_path), "/proc/%d/status", pid);

    int32_t fd[2];
    if (pipe(fd) == -1) {
        return -1;
    }

    execl("/bin/grep", "grep", "PPid:", stat_path, NULL);

    char grep_result[32];
    read(fd[1], grep_result, sizeof(grep_result));

    close(fd[0]);
    close(fd[1]);

    pid_t result;
    sscanf(grep_result, "PPid:%d", &result);

    return result;
}

pid_t find_lca(pid_t x, pid_t y) {
    if (x == y) {
        return x;
    }

    pid_t x_parents[MAX_TREE_DEPTH];
    pid_t y_parents[MAX_TREE_DEPTH];

    uint32_t x_back_idx = 0;
    uint32_t y_back_idx = 0;

    pid_t cur_pid = x;
    while (cur_pid != 1 && x_back_idx < MAX_TREE_DEPTH) {
        x_parents[x_back_idx++] = cur_pid;
        cur_pid = get_parent_pid(x);
    }

    cur_pid = y;
    while (cur_pid != 1 && y_back_idx < MAX_TREE_DEPTH) {
        y_parents[y_back_idx++] = cur_pid;
        cur_pid = get_parent_pid(y);
    }

    --x_back_idx;
    --y_back_idx;

    while (x_parents[x_back_idx] == y_parents[y_back_idx] &&
           x_back_idx > 0 &&
           y_back_idx > 0) {
        --x_back_idx;
        --y_back_idx;
    }

    return x_parents[x_back_idx + 1];
}
