#ifndef EVENTSELECTLOOP_HPP
# define EVENTSELECTLOOP_HPP
# include "asocket.hpp"
# include "isocketlike.hpp"
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

class ISocketLike;

class EventSelectLoop: public IPanopticon {
    private:
        typedef std::map<t_fd, ISocketLike*>            socket_map;
        typedef std::vector< SocketPreservation >   update_queue;
        socket_map                      read_map;
        socket_map                      write_map;
        socket_map                      exception_map;
        update_queue                    up_queue;

        void    prepare_fd_set(socket_map& sockmap, fd_set *sockset);
        void    scan_fd_set(socket_map& sockmap, fd_set *sockset);
        void    preserve(ISocketLike* socket, SocketHolderMapType from, SocketHolderMapType to);
        void    update();

        void    watch(ISocketLike* socket, SocketHolderMapType map_type);
        void    unwatch(ISocketLike* socket, SocketHolderMapType map_type);

        void    destroy_all(socket_map &m);

    public:
        EventSelectLoop();
        ~EventSelectLoop();

        void    loop();
        void    preserve_clear(ISocketLike* socket, SocketHolderMapType from);
        void    preserve_set(ISocketLike* socket, SocketHolderMapType to);
        void    preserve_move(ISocketLike* socket, SocketHolderMapType from, SocketHolderMapType to);
};

#endif
