#include <asm-generic/socket.h>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstddef>
#include <cstdio>
#include <ctime>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

struct Conn {
    int fd_;
    std::string out_;
};

bool would_block(int err) {
    return err == EAGAIN || err == EWOULDBLOCK;
}

double process_cpu_ms() {
    timespec ts; clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts);
    return ts.tv_sec * 1e3 + ts.tv_nsec / 1e6;
}

int main(int argc, char** argv) {
    if (argc < 4) { fprintf(stderr, "usage: %s <port> <run_seconds> <pace_us>\n", argv[0]); return 1;}
    int port = atoi(argv[1]);
    int run_s = atoi(argv[2]);
    int pace_us = atoi(argv[3]);

    signal(SIGPIPE, SIG_IGN);

    int fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    int yes = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);
    sockaddr_in server_addr{};
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); server_addr.sin_family = AF_INET; server_addr.sin_port = htons(port);
    if (bind(fd, (sockaddr*) &server_addr, sizeof server_addr)<0) {perror("bind"); return 1;}
    if (listen(fd, 16) < 0) {perror("listen"); return 1;}

    std::vector<Conn> conns;
    const size_t HIGH_WATER = 1 << 20;
    long passes = 0, idle_passes = 0, accepted = 0;
    double cpu0 = process_cpu_ms();
    auto start = std::chrono::steady_clock::now();
    while(std::chrono::steady_clock::now() - start < std::chrono::seconds(run_s)) {
        ++passes;
        bool progress = false;

        for(;;) {
            sockaddr_in client_addr{};
            socklen_t client_len = sizeof(client_addr);
            int conn_fd = accept4(fd, (sockaddr*) &client_addr, &client_len, SOCK_NONBLOCK);
            if (conn_fd > 0) { conns.push_back({conn_fd, {}}); ++accepted; progress = true; continue;}
            if (errno == EINTR || errno == ECONNABORTED) continue;
            if (!would_block(errno)) perror("accept4");
            break;
        }

        for (size_t i=0; i< conns.size(); ) {
            Conn& conn = conns[i];
            bool dead = false;

            char buff[16384];
            while(!dead && conn.out_.size() < HIGH_WATER) {
                ssize_t n = recv(conn.fd_, &buff, sizeof(buff), 0);
                if (n > 0) { conn.out_.append(buff, n); progress = true; continue; }
                if (n==0) {dead = true; break;}
                if (n<0 && errno == EINTR) continue;
                if (n<0 && !would_block(errno)) dead = true;
                break;
            }

            while(!dead && !conn.out_.empty()) {
                ssize_t n = send(conn.fd_, conn.out_.data(), conn.out_.size(), 0);
                if (n>0) {conn.out_.erase(0, n); progress = true; continue;}
                if (n==0) {dead = true; break;}
                if (n<0 && errno == EINTR) continue;
                if (n<0 && !would_block(errno)) dead = true;
                break;

            }

            if (dead) {
                close(conn.fd_); conns[i] = std::move(conns.back()); conns.pop_back(); progress = true;
            } else {
                i++;
            }
        }
        if (!progress) {
            idle_passes++;
            if (pace_us) usleep(pace_us);
        }
    }
    double wall = (std::chrono::steady_clock::now() - start).count();
    double cpu = process_cpu_ms() - cpu0;

    fprintf(stderr, "STATS: wall=%.2fs cpu=%.1fms (%.1f%%) passes=%ld idle_passes=%ld accepted=%ld\n", wall, cpu, 100.0 * cpu / (wall * 1000.0), passes, idle_passes, accepted);
    for (auto& c : conns) close(c.fd_);
    close(fd);
    return 1;
}