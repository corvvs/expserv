#include "eventkqueueloop.hpp"
#include "eventpollloop.hpp"
#include "eventselectloop.hpp"
#include "httpserver.hpp"

int main() {
    // HTTPServer  http_server(new EventKqueueLoop());
    HTTPServer http_server(new EventPollLoop());
    // HTTPServer  http_server(new EventSelectLoop());

    http_server.listen(SD_IP4, ST_TCP, 8080);
    http_server.listen(SD_IP4, ST_TCP, 8081);
    http_server.listen(SD_IP4, ST_TCP, 8082);
    http_server.listen(SD_IP4, ST_TCP, 8083);
    http_server.listen(SD_IP4, ST_TCP, 8084);

    http_server.run();
}
