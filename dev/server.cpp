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
    EventLoop   el;

    el.listen(SD_IP4, ST_TCP, 8080);
    el.listen(SD_IP4, ST_TCP, 8081);
    el.listen(SD_IP4, ST_TCP, 8082);
    el.listen(SD_IP4, ST_TCP, 8083);
    el.listen(SD_IP4, ST_TCP, 8084);
    el.loop();
}
