#ifndef EVENTSELECTLOOP_HPP
#define EVENTSELECTLOOP_HPP
#include "asocket.hpp"
#include "iobserver.hpp"
#include "isocketlike.hpp"
#include "socket_type.hpp"
#include "socketlistening.hpp"
#include <errno.h>
#include <map>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

class ISocketLike;

// selectを使ったソケット監視者の実装
class EventSelectLoop : public IObserver {
private:
    typedef std::map<t_fd, ISocketLike *> socket_map;
    typedef std::vector<t_socket_reservation> update_queue;
    socket_map read_map;
    socket_map write_map;
    socket_map exception_map;
    update_queue up_queue;

    void prepare_fd_set(socket_map &sockmap, fd_set *sockset);
    void scan_fd_set(socket_map &sockmap, fd_set *sockset, t_time_epoch_ms now);
    void reserve(ISocketLike *socket, t_socket_operation from, t_socket_operation to);
    void update();

    void watch(ISocketLike *socket, t_socket_operation map_type);
    void unwatch(ISocketLike *socket, t_socket_operation map_type);

    void destroy_all(socket_map &m);

public:
    EventSelectLoop();
    ~EventSelectLoop();

    void loop();
    void reserve_clear(ISocketLike *socket, t_socket_operation from);
    void reserve_set(ISocketLike *socket, t_socket_operation to);
    void reserve_transit(ISocketLike *socket, t_socket_operation from, t_socket_operation to);
};

#endif
