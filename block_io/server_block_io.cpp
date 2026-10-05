#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Unexpected arguments!\n");
        return 1;
    }
    uint16_t port = (uint16_t) atoi(argv[1]);

    int listening_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listening_fd < 0) {
        perror("socket!");
        return 1;
    }

    int yes = 1;
    setsockopt(listening_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    if (bind(listening_fd, (sockaddr*) &server_addr, sizeof(server_addr)) < 0) {
        perror("bind!");
        return 1;
    }

    int accept_queue = 16;
    if (listen(listening_fd, accept_queue) < 0) {
        perror("listen!");
        return 1;
    }

    printf("Listening on port %d (one client at a time -- try connecting twice!)\n", port);
    while(true) {
        sockaddr_in client_addr{};
        socklen_t length = (socklen_t)sizeof(client_addr);
        int client_fd = accept(listening_fd, (sockaddr*)&client_addr , &length);
        if (client_fd < 0) { perror("accept"); continue; }

        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, ip, sizeof(ip));
        printf("Client connected: %s:%d\n", ip, ntohs(client_addr.sin_port));

        char buffer[4096];
        while(true) {
            size_t n = recv(client_fd, buffer, sizeof(buffer), 0);
            if (n < 0) { perror("recv"); break; }
            if (n == 0) { printf("Client disconnected (clean close)\n"); break; }

            size_t send_size = 0;
            bool send_failed = false;
            while(send_size < n) {
                size_t send_n = send(client_fd, buffer + send_size, n - send_size , 0);
                if (send_n < 0) {
                    perror("send!");
                    send_failed = true;
                    break;
                }
                send_size += send_n;
            }
            if (send_failed) {
                break;
            }
        }
        close(client_fd);
    }

    return 0;
}