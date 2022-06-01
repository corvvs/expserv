#include "listeningsocket.hpp"
#include "connectedsocket.hpp"
#include "eventselectloop.hpp"
#include "eventpollloop.hpp"
#include "eventkqueueloop.hpp"
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <map>

#define N 1024

int main() {
    // IPanopticon *p = new EventSelectLoop();
    IPanopticon *p = new EventPollLoop();
    // IPanopticon *p = new EventKqueueLoop();

    p->listen(SD_IP4, ST_TCP, 8080);
    p->listen(SD_IP4, ST_TCP, 8081);
    p->listen(SD_IP4, ST_TCP, 8082);
    p->listen(SD_IP4, ST_TCP, 8083);
    p->listen(SD_IP4, ST_TCP, 8084);
    p->loop();

    delete p;
}
