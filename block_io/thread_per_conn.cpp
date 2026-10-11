#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <cstdio>
#include <cstdlib>
#include <netinet/in.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <iostream>

void handle_client(int client_fd) {
    char buffer[4096];
    while(true) {
        int size = recv(client_fd, buffer, sizeof(buffer), 0);
        if (size < 0) perror("recv!"); break;
        if (size == 0) printf("Clean client close %d", client_fd); break;

        ssize_t total_size = 0;
        bool send_failed = false;
        while(total_size < size) {
            ssize_t send_size = send(client_fd, buffer + total_size, size - total_size, 0);
            if (send_size < 0) perror("send!"); send_failed = true; break;
            total_size += send_size;
        }
        if (send_failed) {
            break;
        }
    }
    close(client_fd);
}

int main(int argc, char** argv) {
    int port = argc > 1 ? atoi(argv[1]) : 9000;
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(server_fd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        return 1;
    }

    int backlog = 16;
    if (listen(server_fd, backlog) < 0) {
        perror("listen");
        return 1;
    }
    fprintf(stdout, "server started: %d\n", port);

    while(true) {
        sockaddr_in client_addr{};
        socklen_t length = (socklen_t) sizeof(client_addr);
        int client_fd = accept(server_fd, (sockaddr*) &client_addr, &length);
        if (client_fd < 0) {
            perror("accept");
            continue;
        }
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, ip, INET_ADDRSTRLEN);
        printf("client connected -> %s:%d\n", ip, ntohs(client_addr.sin_port));

        std::thread(handle_client, client_fd).detach();
    }

    return 0;
}