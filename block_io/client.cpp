
#include <arpa/inet.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
int main(int argc, char** argv) {
    if (argc < 3) {
        fprintf(stderr, "usage: %s <ip> <port>\n", argv[0]); 
        return 1;
    }

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket"); return 1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[2]));
    if (inet_pton(AF_INET, argv[1], &server_addr.sin_addr) != 1) {
        fprintf(stderr, "invalid IP address: %s\n", argv[1]);
        return 1;
    }

    if (connect(fd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        return 1;
    }

    char line[4096];
    while(fgets(line, sizeof(line), stdin)) {
        // printf("[DEBUG] Read from stdin: %s (length: %zu)\n", line, strlen(line));
        ssize_t send_size = 0;
        size_t line_size = strlen(line);
        while((size_t)send_size < line_size) {
            ssize_t sent = send(fd, line + send_size, line_size - send_size, 0);
            if (sent < 0) {
                perror("send"); close(fd); return 1;
            }
            send_size += sent;
        }

        char read_buffer[4096];
        ssize_t read = recv(fd, read_buffer, sizeof(read_buffer), 0);
        if (read<=0) { printf("Server closed the connection.\n"); break; }
        // printf("[DEBUG] Read from server: %s (length: %zu)\n", read_buffer, sizeof(read_buffer));
        fwrite(read_buffer, 1, read, stdout);
    }

    close(fd);
    return 0;
}