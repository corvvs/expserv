#include "eventselectloop.hpp"
#include "eventpollloop.hpp"
#include "eventkqueueloop.hpp"
#include "httpserver.hpp"

int main() {
    HTTPServer  http_server(new EventKqueueLoop());

    http_server.listen(SD_IP4, ST_TCP, 8080);
    http_server.listen(SD_IP4, ST_TCP, 8081);
    http_server.listen(SD_IP4, ST_TCP, 8082);
    http_server.listen(SD_IP4, ST_TCP, 8083);
    http_server.listen(SD_IP4, ST_TCP, 8084);

    http_server.run();
}
