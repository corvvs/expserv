#include "asocket.hpp"

static int sockdomain(SocketDomain d) {
    switch (d) {
    case SD_IP4:
        return AF_INET;
    case SD_IP6:
        return AF_INET6;
    default:
        throw std::runtime_error("unexpected asocket domain");
    }
}

static int socktype(SocketType t) {
    switch (t) {
    case ST_TCP:
        return SOCK_STREAM;
    case ST_UDP:
        return SOCK_DGRAM;
    default:
        throw std::runtime_error("unexpected asocket type");
    }
}

ASocket::ASocket() {
    throw std::runtime_error("forbidden");
}

ASocket::ASocket(
    SocketDomain sdomain,
    SocketType stype
): holding(false), run_counter(0) {
    int d = sockdomain(sdomain);
    int t = socktype(stype);

    // DOUT() << "create asocket for: " << d << ", " << t << "..." << std::endl;
    t_fd sock = socket(d, t, 0);
    if (sock == -1) {
        throw std::runtime_error("failed to initialize asocket");
    }
    // DOUT() << "created asocket." << std::endl;
    fd = sock;
    domain = sdomain;
    type = stype;
}

ASocket::ASocket(
    int sock_fd,
    SocketDomain sdomain,
    SocketType stype
): fd(sock_fd), holding(true), run_counter(0) {
    domain = sdomain;
    type = stype;
}

ASocket::ASocket(const ASocket& other) {
    *this = other;
}

ASocket& ASocket::operator=(const ASocket& rhs) {
    if (this != &rhs) {
        fd = rhs.fd;
        domain = rhs.domain;
        type = rhs.type;
        if (rhs.holding) {
            holding = true;
            const_cast<ASocket&>(rhs).holding = false;
        }
    }
    return *this;
}

ASocket::~ASocket() {
    if (holding) {
        destroy();
    }
}

SocketDomain    ASocket::get_domain() const {
    return domain;
}

SocketType      ASocket::get_type() const {
    return type;
}

t_port      ASocket::get_port() const {
    return port;
}

void            ASocket::destroy() {
    // DOUT() << "destroying asocket " << fd << ", " << domain << ", " << type << "..." << std::endl;
    close(fd);
}
