#ifndef EVENTKQUEUELOOP_HPP
# define EVENTKQUEUELOOP_HPP
# include "asocket.hpp"
# include "isocketlike.hpp"
# include "socketlistening.hpp"
# include <map>
# include <vector>
# include <sys/time.h>
# include <sys/types.h>
# include <sys/event.h>
# include <errno.h>
# include <unistd.h>
# include "socket_type.hpp"
# include "iobserver.hpp"

class ISocketLike;

// kqueueを使ったソケット監視者の実装
class EventKqueueLoop: public IObserver {
    private:
        typedef std::map<t_fd, ISocketLike*>            socket_map;
        typedef std::vector< t_socket_reservation >   update_queue;
        typedef struct kevent                       t_kevent;
        typedef std::vector< t_kevent >             event_list;
        typedef short                               t_kfilter;
        typedef int                                 t_kqueue;

        socket_map                                  sockmap;
        update_queue                                upqueue;
        event_list                                  evlist;
        static const int                            nev;
        t_kqueue                                    kq;

        t_kfilter   filter(t_socket_operation t);

        void    prepare_fd_set(socket_map& sockmap, fd_set *sockset);
        void    scan_fd_set(socket_map& sockmap, fd_set *sockset);
        void    preserve(ISocketLike* socket, t_socket_operation from, t_socket_operation to);
        void    update();

        void    watch(ISocketLike* socket, t_socket_operation map_type);
        void    unwatch(ISocketLike* socket, t_socket_operation map_type);

        void    destroy_all(socket_map &m);

    public:
        EventKqueueLoop();
        ~EventKqueueLoop();

        void    run();
        void    preserve_clear(ISocketLike* socket, t_socket_operation from);
        void    preserve_set(ISocketLike* socket, t_socket_operation to);
        void    preserve_move(ISocketLike* socket, t_socket_operation from, t_socket_operation to);
};

#endif
