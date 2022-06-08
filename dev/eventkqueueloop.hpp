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
        typedef std::map<t_fd, ISocketLike*>        socket_map;
        typedef std::vector< t_socket_reservation > update_queue;
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

        void    reserve(ISocketLike* socket, t_socket_operation from, t_socket_operation to);
        void    update();

    public:
        EventKqueueLoop();
        ~EventKqueueLoop();

        void    loop();
        void    reserve_clear(ISocketLike* socket, t_socket_operation from);
        void    reserve_set(ISocketLike* socket, t_socket_operation to);
        void    reserve_transit(ISocketLike* socket, t_socket_operation from, t_socket_operation to);
};

#endif
