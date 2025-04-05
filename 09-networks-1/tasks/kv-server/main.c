#include <fcntl.h>
#include <linux/limits.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>

#define MAX_EVENTS 10
#define BUFF_SIZE  64
#define MAX_CONN   4096

typedef struct StorageItem {
    char                key[PATH_MAX];
    char                value[PATH_MAX];
    struct StorageItem* next;
} StorageItem;

typedef struct Storage {
    struct StorageItem* head;
} Storage;

static StorageItem* find_item(Storage* storage, const char* key) {
    StorageItem* current = storage->head;
    while (current != NULL) {
        if (strncmp(current->key, key, PATH_MAX) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

static void set_item(Storage* storage, const char* key, const char* value) {
    StorageItem* element = find_item(storage, key);
    if (element == NULL) {
        element = malloc(sizeof(StorageItem));
        if (!element) {
            fprintf(stderr, "malloc failed: %s\n", strerror(errno));
            return;
        }
        memset(element, 0, sizeof(StorageItem));
        strncpy(element->key, key, PATH_MAX - 1);
        element->next = storage->head;
        storage->head = element;
    }
    strncpy(element->value, value, PATH_MAX - 1);
    element->value[PATH_MAX - 1] = '\0';
}

static const char* get_item(Storage* storage, const char* key) {
    StorageItem* element = find_item(storage, key);
    if (element == NULL) {
        return "";
    } else {
        return element->value;
    }
}

static int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        fprintf(stderr, "fcntl(F_GETFL): %s\n", strerror(errno));
        return -1;
    }

    flags |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flags) == -1) {
        fprintf(stderr, "fcntl(F_SETFL): %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

static int epoll_ctl_add(int epoll_fd, int socket_fd, uint32_t events) {
    struct epoll_event event;
    memset(&event, 0, sizeof(event));
    event.data.fd = socket_fd;
    event.events  = events;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socket_fd, &event) == -1) {
        fprintf(stderr, "epoll_ctl_add failed: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

static int request_handle(char* buff, char* answer, Storage* storage) {
    char*      first_save_ptr;
    const char delim_space[]    = " ";
    const char delim_new_line[] = "\n";

    char* token = strtok_r(buff, delim_space, &first_save_ptr);
    if (token == NULL) {
        fprintf(stderr, "No command found in request\n");
        return -1;
    } else if (strncmp(token, "get", 3) == 0) {
        char* key = strtok_r(NULL, delim_new_line, &first_save_ptr);
        if (!key) {
            snprintf(answer, BUFF_SIZE, "\n");
            return 0;
        }

        const char* data = get_item(storage, key);
        snprintf(answer, BUFF_SIZE, "%.*s\n", BUFF_SIZE - 2, data);
        return 0;
    } else if (strncmp(token, "set", 3) == 0) {
        char* key   = strtok_r(NULL, delim_space, &first_save_ptr);
        char* value = strtok_r(NULL, delim_new_line, &first_save_ptr);

        if (!key || !value) {
            fprintf(stderr, "Invalid set command\n");
            return -1;
        }

        set_item(storage, key, value);
        answer[0] = '\0';
        return 0;
    } else {
        fprintf(stderr, "Unknown command: %s\n", token);
    }
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    Storage* storage = malloc(sizeof(Storage));
    if (!storage) {
        fprintf(stderr, "malloc failed for storage: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }
    storage->head = NULL;

    int server_port  = atoi(argv[1]);

    char buff[BUFF_SIZE];
    char answer[BUFF_SIZE];

    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        fprintf(stderr, "socket: %s\n", strerror(errno));
        free(storage);
        return EXIT_FAILURE;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons(server_port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        fprintf(stderr, "bind: %s\n", strerror(errno));
        close(server_socket);
        free(storage);
        return EXIT_FAILURE;
    }

    if (set_nonblocking(server_socket) == -1) {
        close(server_socket);
        free(storage);
        return EXIT_FAILURE;
    }

    if (listen(server_socket, MAX_CONN) == -1) {
        fprintf(stderr, "listen: %s\n", strerror(errno));
        close(server_socket);
        free(storage);
        return EXIT_FAILURE;
    }

    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        fprintf(stderr, "epoll_create1: %s\n", strerror(errno));
        close(server_socket);
        free(storage);
        return EXIT_FAILURE;
    }

    if (epoll_ctl_add(epoll_fd, server_socket, EPOLLIN) == -1) {
        close(server_socket);
        close(epoll_fd);
        free(storage);
        return EXIT_FAILURE;
    }

    struct epoll_event epoll_events[MAX_EVENTS];

    for (;;) {
        int nfds = epoll_wait(epoll_fd, epoll_events, MAX_EVENTS, -1);
        if (nfds == -1) {
            fprintf(stderr, "epoll_wait: %s\n", strerror(errno));
            break;
        }

        for (int i = 0; i < nfds; ++i) {
            if (epoll_events[i].data.fd == server_socket) {
                for (;;) {
                    int connection = accept(server_socket, NULL, NULL);
                    if (connection == -1) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            break;
                        } else {
                            fprintf(stderr, "accept: %s\n", strerror(errno));
                            break;
                        }
                    }

                    if (set_nonblocking(connection) == -1) {
                        close(connection);
                        continue;
                    }

                    if (epoll_ctl_add(epoll_fd, connection, EPOLLIN | EPOLLET | EPOLLRDHUP) == -1) {
                        close(connection);
                        continue;
                    }
                }

            } else if (epoll_events[i].events & EPOLLIN) {
                int client_fd = epoll_events[i].data.fd;
                memset(buff, 0, sizeof(buff));
                memset(answer, 0, sizeof(answer));

                int request = read(client_fd, buff, sizeof(buff));
                if (request <= 0) {
                    if (request < 0) {
                        fprintf(stderr, "read: %s\n", strerror(errno));
                    }
                    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL) == -1) {
                        fprintf(stderr, "epoll_ctl_del: %s\n", strerror(errno));
                    }
                    close(client_fd);
                    continue;
                }

                if (request_handle(buff, answer, storage) == -1) {
                    fprintf(stderr, "Failed to handle request\n");
                }

                if (strlen(answer) > 0) {
                    int written = write(client_fd, answer, strlen(answer));
                    if (written == -1) {
                        fprintf(stderr, "write: %s\n", strerror(errno));
                    }
                }

            } else if (epoll_events[i].events & EPOLLRDHUP) {
                int client_fd = epoll_events[i].data.fd;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL) == -1) {
                    fprintf(stderr, "epoll_ctl_del: %s\n", strerror(errno));
                }
                close(client_fd);
            }
        }
    }

    close(server_socket);
    close(epoll_fd);
    free(storage);

    return 0;
}
