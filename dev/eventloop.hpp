#ifndef SOCKETHOLDER_HPP
# define SOCKETHOLDER_HPP
# include "isocket.hpp"
# include <map>
# include <vector>
# include <sys/select.h>
# include <sys/time.h>
# include <sys/types.h>
# include <errno.h>
# include <unistd.h>

class ISocket;

enum SocketHolderMapType {
    SHMT_NONE,
    SHMT_READ,
    SHMT_WRITE,
    SHMT_EXCEPTION
};

struct SocketPreservation {
    ISocket* sock;
    SocketHolderMapType from;
    SocketHolderMapType to;
};

class EventLoop {
    public:
        typedef std::map<int, ISocket*>  socket_map;
        typedef std::vector< SocketPreservation > update_queue;

    private:
        socket_map                      read_map;
        socket_map                      write_map;
        socket_map                      exception_map;
        update_queue                    up_queue;

        void    prepare_fd_set(socket_map& sockmap, fd_set *sockset);
        void    scan_fd_set(socket_map& sockmap, fd_set *sockset);
        void    preserve(ISocket* socket, SocketHolderMapType from, SocketHolderMapType to);
        void    update();

    public:
        EventLoop();
        ~EventLoop();

        void    watch(ISocket* socket, SocketHolderMapType map_type);
        void    unwatch(ISocket* socket, SocketHolderMapType map_type);

        void    loop();

        void    preserve_clear(ISocket* socket, SocketHolderMapType from);
        void    preserve_set(ISocket* socket, SocketHolderMapType to);
        void    preserve_move(ISocket* socket, SocketHolderMapType from, SocketHolderMapType to);
};

#endif
