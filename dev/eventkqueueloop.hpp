#ifndef EVENTKQUEUELOOP_HPP
# define EVENTKQUEUELOOP_HPP
# include "asocket.hpp"
# include "isocketlike.hpp"
# include "listeningsocket.hpp"
# include <map>
# include <vector>
# include <sys/time.h>
# include <sys/types.h>
# include <sys/event.h>
# include <errno.h>
# include <unistd.h>
# include "socket_type.hpp"
# include "ipanopticon.hpp"

class ISocketLike;

class EventKqueueLoop: public IPanopticon {
    private:
        typedef std::map<t_fd, ISocketLike*>            socket_map;
        typedef std::vector< SocketPreservation >   update_queue;
        typedef struct kevent                       t_kevent;
        typedef std::vector< t_kevent >             event_list;
        typedef short                               t_kfilter;
        typedef int                                 t_kqueue;

        socket_map                                  sockmap;
        update_queue                                upqueue;
        event_list                                  evlist;
        static const int                            nev;
        t_kqueue                                    kq;

        t_kfilter   filter(SocketHolderMapType t);

        void    prepare_fd_set(socket_map& sockmap, fd_set *sockset);
        void    scan_fd_set(socket_map& sockmap, fd_set *sockset);
        void    preserve(ISocketLike* socket, SocketHolderMapType from, SocketHolderMapType to);
        void    update();

        void    watch(ISocketLike* socket, SocketHolderMapType map_type);
        void    unwatch(ISocketLike* socket, SocketHolderMapType map_type);

        void    destroy_all(socket_map &m);

    public:
        EventKqueueLoop();
        ~EventKqueueLoop();

        void    loop();
        void    preserve_clear(ISocketLike* socket, SocketHolderMapType from);
        void    preserve_set(ISocketLike* socket, SocketHolderMapType to);
        void    preserve_move(ISocketLike* socket, SocketHolderMapType from, SocketHolderMapType to);
};

#endif
