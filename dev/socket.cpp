#include "socket.hpp"

static int sockdomain(SocketDomain d) {
    switch (d) {
    case SD_IP4:
        return AF_INET;
    case SD_IP6:
        return AF_INET6;
    default:
        throw std::runtime_error("unexpected socket domain");
    }
}

static int socktype(SocketType t) {
    switch (t) {
    case ST_TCP:
        return SOCK_STREAM;
    case ST_UDP:
        return SOCK_DGRAM;
    default:
        throw std::runtime_error("unexpected socket type");
    }
}

Socket::Socket() {
    throw std::runtime_error("forbidden");
}

Socket::Socket(
    SocketDomain sdomain,
    SocketType stype
): holding(false) {
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
    int sock_fd,
    SocketDomain sdomain,
    SocketType stype
): fd(sock_fd), holding(true) {
    domain = sdomain;
    type = stype;
}

Socket::Socket(const Socket& other) {
    *this = other;
}

Socket& Socket::operator=(const Socket& rhs) {
    if (this != &rhs) {
        fd = rhs.fd;
        domain = rhs.domain;
        type = rhs.type;
        if (rhs.holding) {
            holding = true;
            const_cast<Socket&>(rhs).holding = false;
        }
    }
    return *this;
}

Socket::~Socket() {
    if (holding) {
        DOUT() << "destroying socket " << fd << ", " << domain << ", " << type << "..." << std::endl;
        close(fd);
    }
}

int             Socket::get_fd() const {
    return fd;
}

SocketDomain    Socket::get_domain() const {
    return domain;
}

SocketType      Socket::get_type() const {
    return type;
}
