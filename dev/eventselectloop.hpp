#ifndef EVENTSELECTLOOP_HPP
# define EVENTSELECTLOOP_HPP
# include "asocket.hpp"
# include "isocket.hpp"
# include "listeningsocket.hpp"
# include <map>
# include <vector>
# include <sys/select.h>
# include <sys/time.h>
# include <sys/types.h>
# include <errno.h>
# include <unistd.h>
# include "socket_type.hpp"
# include "ipanopticon.hpp"

class ISocket;

class EventSelectLoop: public IPanopticon {
    private:
        typedef std::map<t_fd, ISocket*>            socket_map;
        typedef std::vector< SocketPreservation >   update_queue;
        socket_map                      read_map;
        socket_map                      write_map;
        socket_map                      exception_map;
        update_queue                    up_queue;

        void    prepare_fd_set(socket_map& sockmap, fd_set *sockset);
        void    scan_fd_set(socket_map& sockmap, fd_set *sockset);
        void    preserve(ISocket* socket, SocketHolderMapType from, SocketHolderMapType to);
        void    update();

        void    watch(ISocket* socket, SocketHolderMapType map_type);
        void    unwatch(ISocket* socket, SocketHolderMapType map_type);

        void    destroy_all(socket_map &m);

    public:
        EventSelectLoop();
        ~EventSelectLoop();

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
