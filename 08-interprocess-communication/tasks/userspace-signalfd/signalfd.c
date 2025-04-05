#define _GNU_SOURCE

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>


int signalfd();

int pipe_fd[2];

void signal_handle(int seg) {
  int32_t signal_number = (int32_t)seg;
  if (write(pipe_fd[1], &signal_number, sizeof(signal_number)) == -1) {
    perror("write");
  }
}

int signalfd() {
  if (pipe(pipe_fd) == -1) {
    perror("pipe");
    return -1;
  }

  for (size_t i = 1; i < NSIG; ++i) {
    if (i == SIGKILL || i == SIGSTOP) {
      continue;
    }
    signal(i, signal_handle);
  }


  return pipe_fd[0];
}