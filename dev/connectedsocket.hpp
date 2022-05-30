#ifndef CONNECTEDSOCKET_HPP
# define CONNECTEDSOCKET_HPP
# include "asocket.hpp"
# include "isocket.hpp"
# include "eventloop.hpp"

class ListeningSocket;

class ConnectedSocket: public ASocket, public ISocket {
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
    int get_fd() const;

    void    notify(EventLoop& loop);

private:
    std::string receipt_str;
};

#endif
