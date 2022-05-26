#include "connectedsocket.hpp"

int main() {
    ConnectedSocket sock = ConnectedSocket::connect(SD_IP4, ST_TCP, 8080);
    std::cout << sock.get_fd() << std::endl;

    sock.send("hello\n", 6, 0);
}
