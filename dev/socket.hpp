#ifndef SOCKET_HPP
# define SOCKET_HPP
# include <iostream>
# include <string>
# include <exception>
# include <stdexcept>
# include <sys/types.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <unistd.h>
# include <netdb.h>
# include "test_common.hpp"


enum SocketDomain {
    SD_IP4,
    SD_IP6
};

enum SocketType {
    ST_TCP,
    ST_UDP
};

typedef uint16_t    t_port;
typedef uint32_t    t_addressv4;

class Socket {

private:
    int             fd;
    SocketDomain    domain;
    SocketType      type;

public:

    Socket(
        SocketDomain sdomain,
        SocketType stype
    );

    Socket(
        int fd,
        Socket& listening
    );

    ~Socket();

    int     get_fd() const;

    static Socket   bind(
        SocketDomain sdomain,
        SocketType stype,
        t_port port
    );
    static Socket   connect(
        SocketDomain sdomain,
        SocketType stype,
        t_port port
    );

    void    listen(int backlog);
    Socket  accept();
    ssize_t send(const void *buffer, size_t len, int flags);
    ssize_t receive(void *buffer, size_t len, int flags);
};

#endif
