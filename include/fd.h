#include <unistd.h>
class Fd {
public:
    Fd(int fd) : fd_(fd) {};
    ~Fd() {
        if (fd_ >= 0) {
            close(fd_);
        }
    }
    Fd(Fd& other) = delete;
    Fd(Fd&& other) {
        fd_ = other.fd_;
        other.fd_ = -1;
    }
    Fd& operator=(const Fd& other) = delete;
    Fd& operator=(Fd&& other) {
        if(other.fd_ != 0) {
            close(fd_);
            fd_ = other.fd_;
            other.fd_ = -1;
        }
        return *this;
    }

    int get() {
        return fd_;
    }
private:
int fd_;
};