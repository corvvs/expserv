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

enum t_socket_domain {
    SD_IP4,
    SD_IP6
};

enum t_socket_type {
    ST_TCP,
    ST_UDP
};

typedef int	        t_fd;
typedef uint16_t    t_port;
typedef uint32_t    t_addressv4;

enum t_socket_operation {
    SHMT_NONE,
    SHMT_READ,
    SHMT_WRITE,
    SHMT_EXCEPTION
};

class ISocketLike;

struct t_socket_reservation {
    ISocketLike* sock;
    t_socket_operation from;
    t_socket_operation to;
};

class EventSelectLoop;

#endif
