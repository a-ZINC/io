#pragma once
#include "./fd.h"
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <ratio>
#include <string>

namespace blockingio {
    double read_file_ms(const char* path) {
        Fd fd(open(path, O_RDONLY));
        if (fd.get() < 0) {
            fprintf(stderr, "error: %s\n", strerror(errno));
            exit(-1);
        }

        std::string output;
        size_t n = 0;
        size_t total = 0;
        char buffer[1<<20];
        auto start = std::chrono::high_resolution_clock::now();
        while(true) {
            n = read(fd.get(), buffer, sizeof(buffer));
            if (n < 0) {
                if (errno == EINTR) {
                    continue;
                }
                perror("read failed");
                break;
            } else if (n == 0) {
                break;
            } else {
                output.append(buffer, n);
            }
        }
        return std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - start).count();
    }

    int run(const char* path) {
        double first = read_file_ms(path);
        printf("First read  (cold-ish): %.3f us\n", first * 1e6);

        double second = read_file_ms(path);
        printf("Second read (warm):     %.3f us\n", second * 1e6);

        printf("Speedup: %.1fx\n", first / second);
        return 1;
    }
}