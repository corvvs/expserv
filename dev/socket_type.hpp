#ifndef SOCKET_TYPE_HPP
# define SOCKET_TYPE_HPP
# include <cstdlib>
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
# include <fcntl.h>

enum SocketDomain {
    SD_IP4,
    SD_IP6
};

enum SocketType {
    ST_TCP,
    ST_UDP
};

typedef int	        t_fd;
typedef uint16_t    t_port;
typedef uint32_t    t_addressv4;

enum SocketHolderMapType {
    SHMT_NONE,
    SHMT_READ,
    SHMT_WRITE,
    SHMT_EXCEPTION
};

class ISocketLike;

struct SocketPreservation {
    ISocketLike* sock;
    SocketHolderMapType from;
    SocketHolderMapType to;
};

class EventSelectLoop;

#endif
