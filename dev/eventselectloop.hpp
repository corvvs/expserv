#ifndef EVENTSELECTLOOP_HPP
# define EVENTSELECTLOOP_HPP
# include "asocket.hpp"
# include "isocketlike.hpp"
# include "socketlistening.hpp"
# include <map>
# include <vector>
# include <sys/select.h>
# include <sys/time.h>
# include <sys/types.h>
# include <errno.h>
# include <unistd.h>
# include "socket_type.hpp"
# include "iobserver.hpp"

class ISocketLike;

// selectを使ったソケット監視者の実装
class EventSelectLoop: public IObserver {
    private:
        typedef std::map<t_fd, ISocketLike*>            socket_map;
        typedef std::vector< t_socket_reservation >   update_queue;
        socket_map                      read_map;
        socket_map                      write_map;
        socket_map                      exception_map;
        update_queue                    up_queue;

        void    prepare_fd_set(socket_map& sockmap, fd_set *sockset);
        void    scan_fd_set(socket_map& sockmap, fd_set *sockset);
        void    preserve(ISocketLike* socket, t_socket_operation from, t_socket_operation to);
        void    update();

        void    watch(ISocketLike* socket, t_socket_operation map_type);
        void    unwatch(ISocketLike* socket, t_socket_operation map_type);

        void    destroy_all(socket_map &m);

    public:
        EventSelectLoop();
        ~EventSelectLoop();

        void    run();
        void    preserve_clear(ISocketLike* socket, t_socket_operation from);
        void    preserve_set(ISocketLike* socket, t_socket_operation to);
        void    preserve_move(ISocketLike* socket, t_socket_operation from, t_socket_operation to);
};

#endif
