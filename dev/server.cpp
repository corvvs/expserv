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

int main() {
    // IPanopticon *p = new EventSelectLoop();
    IPanopticon *p = new EventPollLoop();
    // IPanopticon *p = new EventKqueueLoop();

    std::vector<Channel*> channels;
    channels.push_back(Channel::listen(SD_IP4, ST_TCP, 8080));
    channels.push_back(Channel::listen(SD_IP4, ST_TCP, 8081));
    channels.push_back(Channel::listen(SD_IP4, ST_TCP, 8082));
    channels.push_back(Channel::listen(SD_IP4, ST_TCP, 8083));
    channels.push_back(Channel::listen(SD_IP4, ST_TCP, 8084));
    for (std::vector<Channel*>::iterator it = channels.begin(); it != channels.end(); it++) {
        p->preserve_set(*it, SHMT_READ);
    }

    p->loop();

    delete p;
}
