#include "eventkqueueloop.hpp"
#include "eventpollloop.hpp"
#include "eventselectloop.hpp"
#include "httpserver.hpp"
#define SIZE_ERROR(predicate)                                                                                          \
    do {                                                                                                               \
        if (predicate) {                                                                                               \
            throw std::runtime_error(#predicate);                                                                      \
        }                                                                                                              \
    } while (0)

void assert_sizeoftype() {
    SIZE_ERROR(sizeof(u8t) != 1);
    SIZE_ERROR(sizeof(u16t) != 2);
    SIZE_ERROR(sizeof(u32t) != 4);
    SIZE_ERROR(sizeof(u64t) != 8);
}

int main() {
    assert_sizeoftype();

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
