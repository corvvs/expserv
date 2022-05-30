#include "listeningsocket.hpp"
#include "connectedsocket.hpp"
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <map>

#define N 1024

int main() {
    ListeningSocket* sock = ListeningSocket::bind(SD_IP4, ST_TCP, 8080);
    std::cout << sock->get_fd() << std::endl;
    sock->listen(128);
    EventLoop   loop;
    loop.watch(sock, SHMT_READ);
    while (1) {
        loop.loop();
    }
}
