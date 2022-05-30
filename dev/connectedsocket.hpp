#ifndef CONNECTEDSOCKET_HPP
# define CONNECTEDSOCKET_HPP
# include "socket.hpp"
# include <fcntl.h>

class ListeningSocket;

class ConnectedSocket: public Socket {
private:
    ConnectedSocket();
    ConnectedSocket(
        SocketDomain sdomain,
        SocketType stype
    );
    ConnectedSocket(
        int fd,
        ListeningSocket& listening
    );

public:
    friend class ListeningSocket;

    ConnectedSocket(const ConnectedSocket& other);
    ConnectedSocket& operator=(const ConnectedSocket& rhs);

    static ConnectedSocket  *connect(
        SocketDomain sdomain,
        SocketType stype,
        t_port port
    );
    ssize_t send(const void *buffer, size_t len, int flags);
    ssize_t receive(void *buffer, size_t len, int flags);

    void    run(EventLoop& loop);

private:
    std::string receipt_str;
};

#endif
