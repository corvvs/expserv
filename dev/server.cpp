#include "socketlistening.hpp"
#include "socketconnected.hpp"
#include "eventselectloop.hpp"
#include "eventpollloop.hpp"
#include "eventkqueueloop.hpp"
#include "channel.hpp"
#include "httpserver.hpp"
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <map>

int main() {
    // HTTPServer<EventSelectLoop>  server;
    HTTPServer<EventPollLoop>  server;
    // HTTPServer<EventKqueueLoop>  server;

    server.listen(SD_IP4, ST_TCP, 8080);
    server.listen(SD_IP4, ST_TCP, 8081);
    server.listen(SD_IP4, ST_TCP, 8082);
    server.listen(SD_IP4, ST_TCP, 8083);
    server.listen(SD_IP4, ST_TCP, 8084);

    server.loop();
}
