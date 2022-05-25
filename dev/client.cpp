#include "socket.hpp"

int main() {
    Socket sock = Socket::connect(SD_IP4, ST_TCP, 8080);
    std::cout << sock.get_fd() << std::endl;

    sock.send("hello\n", 6, 0);
}
