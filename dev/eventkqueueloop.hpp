#ifndef EVENTKQUEUELOOP_HPP
# define EVENTKQUEUELOOP_HPP
# include "asocket.hpp"
# include "isocket.hpp"
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

class ISocket;

class EventKqueueLoop: public IPanopticon {
    private:
        typedef std::map<t_fd, ISocket*>            socket_map;
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
        void    preserve(ISocket* socket, SocketHolderMapType from, SocketHolderMapType to);
        void    update();

        void    watch(ISocket* socket, SocketHolderMapType map_type);
        void    unwatch(ISocket* socket, SocketHolderMapType map_type);

        void    destroy_all(socket_map &m);

    public:
        EventKqueueLoop();
        ~EventKqueueLoop();

        void    listen(
            SocketDomain sdomain,
            SocketType stype,
            t_port port
        );

        void    loop();
        void    preserve_clear(ISocket* socket, SocketHolderMapType from);
        void    preserve_set(ISocket* socket, SocketHolderMapType to);
        void    preserve_move(ISocket* socket, SocketHolderMapType from, SocketHolderMapType to);
};

#endif
