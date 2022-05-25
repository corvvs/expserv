#include "socket.hpp"

int sockdomain(SocketDomain d) {
    switch (d) {
    case SD_IP4:
        return AF_INET;
    case SD_IP6:
        return AF_INET6;
    default:
        throw std::runtime_error("unexpected socket domain");
    }
}

int socktype(SocketType t) {
    switch (t) {
    case ST_TCP:
        return SOCK_STREAM;
    case ST_UDP:
        return SOCK_DGRAM;
    default:
        throw std::runtime_error("unexpected socket type");
    }
}

void cpp_bzero(void *mem, size_t n) {
    for (size_t i = 0; i < n; i++) {
        ((unsigned char*)mem)[i] = 0;
    }
}

Socket::Socket(
        SocketDomain sdomain,
        SocketType stype
) {
    int d = sockdomain(sdomain);
    int t = socktype(stype);

    DOUT() << "create socket for: " << d << ", " << t << "..." << std::endl;
    int sock = socket(d, t, 0);
    if (sock == -1) {
        throw std::runtime_error("failed to initialize socket");
    }
    DOUT() << "created socket." << std::endl;
    fd = sock;
    domain = sdomain;
    type = stype;
}

Socket::Socket(
    int accepted_fd,
    Socket& listening_socket
): fd(accepted_fd) {
    domain = listening_socket.domain;
    type = listening_socket.type;
}

Socket::~Socket() {
    DOUT() << "destroying socket " << fd << ", " << domain << ", " << type << "..." << std::endl;
    close(fd);
}


int     Socket::get_fd() const {
    return fd;
}

Socket Socket::bind(
    SocketDomain sdomain,
    SocketType stype,
    t_port port
) {

    Socket sock = Socket(sdomain, stype);
    int fd = sock.get_fd();

    struct sockaddr_in sa;
    cpp_bzero(&sa, sizeof(sa));
    int d = sockdomain(sdomain);
    sa.sin_family = d;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = htonl(INADDR_ANY);
    DOUT() << "binding socket for: " << port << ", " << sa.sin_addr.s_addr << "..." << std::endl;
    if (::bind(fd, (struct sockaddr*) &sa, sizeof(struct sockaddr_in)) == -1) {
        throw std::runtime_error("failed to bind a socket");
    }
    DOUT() << "bound socket." << std::endl;
    return sock;
}

Socket Socket::connect(
    SocketDomain sdomain,
    SocketType stype,
    t_port port
) {

    Socket sock = Socket(sdomain, stype);
    int fd = sock.get_fd();

    struct sockaddr_in sa;
    cpp_bzero(&sa, sizeof(sa));
    int d = sockdomain(sdomain);
    sa.sin_family = d;
    sa.sin_port = htons(port);

    // localhost の IP アドレスを引く
    struct hostent *hostent = gethostbyname("localhost");
    if (hostent == NULL) {
        throw std::runtime_error("failed to gethostbyname");
    }
    memcpy(&sa.sin_addr, hostent->h_addr_list[0], sizeof(sa.sin_addr));
    // 接続
    if (::connect(fd, (struct sockaddr*) &sa, sizeof(struct sockaddr_in)) == -1)
    {
        throw std::runtime_error("failed to connect");
    }
    return sock;
}

void    Socket::listen(int backlog) {
    DOUT() << "making socket listening in backlog: " << backlog << std::endl;
    if (::listen(fd, backlog) == -1) {
        throw std::runtime_error("failed to listen");
    }
    DOUT() << "now listening..." << std::endl;
}

Socket  Socket::accept() {
    int accepted_fd = ::accept(fd, NULL, NULL);
    DOUT() << "accepted" << std::endl;
    if (accepted_fd < 0) {
        throw std::runtime_error("failed to accept");
    }
    return Socket(accepted_fd, *this);
}

ssize_t Socket::send(const void *buffer, size_t len, int flags) {
    return ::send(fd, buffer, len, flags);
}

ssize_t Socket::receive(void *buffer, size_t len, int flags) {
    return ::recv(fd, buffer, len, flags);
}
