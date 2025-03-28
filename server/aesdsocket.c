#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <syslog.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/file.h>

#define PORT 9000
#define BACKLOG 10
#define BUFFER_SIZE 4096
#define FILE_PATH "/var/tmp/aesdsocketdata"

int server_socket = -1;
int client_socket = -1;
int file_fd = -1;

void cleanup() {
    if (client_socket != -1) {
        close(client_socket);
    }
    if (server_socket != -1) {
        close(server_socket);
    }
    if (file_fd != -1) {
        close(file_fd);
        remove(FILE_PATH);
    }
    syslog(LOG_INFO, "Caught signal, exiting");
    closelog();
}

void signal_handler(int signo) {
    cleanup();
    exit(0);
}

void daemonize() {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    if (setsid() < 0) {
        perror("setsid");
        exit(EXIT_FAILURE);
    }

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    open("/dev/null", O_RDONLY);
    open("/dev/null", O_WRONLY);
    open("/dev/null", O_RDWR);
}

int main(int argc, char *argv[]) {
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;
    int opt = 1;

    openlog("aesdsocket", LOG_PID | LOG_CONS, LOG_USER);

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    if (argc == 2 && strcmp(argv[1], "-d") == 0) {
        daemonize();
    }

    if (access(FILE_PATH, F_OK) == 0) {
        if (remove(FILE_PATH) != 0) {
            syslog(LOG_ERR, "Failed to delete existing file: %s", strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        syslog(LOG_ERR, "Socket creation failed: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        syslog(LOG_ERR, "Setsockopt failed: %s", strerror(errno));
        cleanup();
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        syslog(LOG_ERR, "Bind failed: %s", strerror(errno));
        cleanup();
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, BACKLOG) == -1) {
        syslog(LOG_ERR, "Listen failed: %s", strerror(errno));
        cleanup();
        exit(EXIT_FAILURE);
    }

    syslog(LOG_INFO, "Server started on port %d", PORT);

    while (1) {
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket == -1) {
            syslog(LOG_ERR, "Accept failed: %s", strerror(errno));
            continue;
        }

        syslog(LOG_INFO, "Accepted connection from %s", inet_ntoa(client_addr.sin_addr));

        file_fd = open(FILE_PATH, O_CREAT | O_APPEND | O_RDWR, 0644);
        if (file_fd == -1) {
            syslog(LOG_ERR, "File open failed: %s", strerror(errno));
            close(client_socket);
            client_socket = -1;
            continue;
        }

        char temp_buffer[BUFFER_SIZE];
        size_t temp_buffer_len = 0;

        while ((bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0)) > 0) {
            for (ssize_t i = 0; i < bytes_received; i++) {
                temp_buffer[temp_buffer_len++] = buffer[i];

                if (temp_buffer_len == BUFFER_SIZE || buffer[i] == '\n') {
                    if (flock(file_fd, LOCK_EX) == -1) {
                        syslog(LOG_ERR, "File lock failed: %s", strerror(errno));
                        temp_buffer_len = 0;
                        break;
                    }

                    if (write(file_fd, temp_buffer, temp_buffer_len) != temp_buffer_len) {
                        syslog(LOG_ERR, "File write failed: %s", strerror(errno));
                        flock(file_fd, LOCK_UN);
                        temp_buffer_len = 0;
                        break;
                    }

                    if (flock(file_fd, LOCK_UN) == -1) {
                        syslog(LOG_ERR, "File unlock failed: %s", strerror(errno));
                        temp_buffer_len = 0;
                        break;
                    }

                    if (buffer[i] == '\n') {
                        if (lseek(file_fd, 0, SEEK_SET) == -1) {
                            syslog(LOG_ERR, "File rewind failed: %s", strerror(errno));
                            temp_buffer_len = 0;
                            break;
                        }

                        ssize_t read_bytes;
                        while ((read_bytes = read(file_fd, buffer, BUFFER_SIZE)) > 0) {
                            ssize_t total_sent = 0;
                            while (total_sent < read_bytes) {
                                ssize_t sent = send(client_socket, buffer + total_sent, read_bytes - total_sent, 0);
                                if (sent == -1) {
                                    syslog(LOG_ERR, "Send failed: %s", strerror(errno));
                                    break;
                                }
                                total_sent += sent;
                            }
                        }

                        if (read_bytes == -1) {
                            syslog(LOG_ERR, "File read failed: %s", strerror(errno));
                            temp_buffer_len = 0;
                            break;
                        }
                    }

                    temp_buffer_len = 0;
                }
            }
        }

        if (bytes_received == -1) {
            syslog(LOG_ERR, "Receive failed: %s", strerror(errno));
        }

        close(file_fd);
        file_fd = -1;

        syslog(LOG_INFO, "Closed connection from %s", inet_ntoa(client_addr.sin_addr));
        close(client_socket);
        client_socket = -1;
    }

    cleanup();
    return 0;
}

