#ifndef CONNECTEDSOCKET_HPP
# define CONNECTEDSOCKET_HPP
# include "asocket.hpp"
# include "ipanopticon.hpp"

class SocketListening;

class SocketConnected: public ASocket {
private:
    SocketConnected();
    SocketConnected(
        t_socket_domain sdomain,
        t_socket_type stype
    );
    SocketConnected(
        t_fd fd,
        SocketListening& listening
    );

public:
    friend class SocketListening;

    SocketConnected(const SocketConnected& other);
    SocketConnected& operator=(const SocketConnected& rhs);

    static SocketConnected  *connect(
        t_socket_domain sdomain,
        t_socket_type stype,
        t_port port
    );
    ssize_t send(const void *buffer, size_t len, int flags);
    ssize_t receive(void *buffer, size_t len, int flags);
    int get_fd() const;
};

#endif
