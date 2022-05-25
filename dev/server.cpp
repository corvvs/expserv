#include "socket.hpp"

#define N 1024

int main() {
    Socket sock = Socket::bind(SD_IP4, ST_TCP, 8080);
    std::cout << sock.get_fd() << std::endl;

    sock.listen(128);

    while (1) {
        Socket accepted = sock.accept();
        std::cout << "!!accepted!!" << std::endl;
        char buf[N];
        while (1) {
            ssize_t receipt = accepted.receive(&buf, N, 0);
            if (receipt <= 0) {
                break;
            }
            write(STDOUT_FILENO, buf, receipt);
        }
    }
}
