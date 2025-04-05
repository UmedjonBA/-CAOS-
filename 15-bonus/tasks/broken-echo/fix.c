#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>

void fix_broken_echo() {
    pid_t ppid = getppid();
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/fd/3", ppid);

    int fd_in = open(path, O_RDONLY);
    if (fd_in < 0) {
        perror("open input");
        exit(1);
    }

    int fd_out = open("/proc/self/fd/1", O_WRONLY);
    if (fd_out < 0) {
        perror("open output");
        exit(1);
    }

    char buf[4096];
    ssize_t n;
    while ((n = read(fd_in, buf, sizeof(buf))) > 0) {
        ssize_t written = 0;
        while (written < n) {
            ssize_t w = write(fd_out, buf + written, n - written);
            if (w < 0) {
                perror("write");
                exit(1);
            }
            written += w;
        }
    }

    if (n < 0) {
        perror("read");
        exit(1);
    }
}
