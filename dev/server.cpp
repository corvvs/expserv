#include "socketlistening.hpp"
#include "socketconnected.hpp"
#include "eventselectloop.hpp"
#include "eventpollloop.hpp"
#include "eventkqueueloop.hpp"
#include "channel.hpp"
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

    p->preserve_set(Channel::listen(SD_IP4, ST_TCP, 8080), SHMT_READ);
    p->preserve_set(Channel::listen(SD_IP4, ST_TCP, 8081), SHMT_READ);
    p->preserve_set(Channel::listen(SD_IP4, ST_TCP, 8082), SHMT_READ);
    p->preserve_set(Channel::listen(SD_IP4, ST_TCP, 8083), SHMT_READ);
    p->preserve_set(Channel::listen(SD_IP4, ST_TCP, 8084), SHMT_READ);

    p->loop();

    delete p;
}
