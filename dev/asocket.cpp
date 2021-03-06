#include "asocket.hpp"

static int sockdomain(t_socket_domain d) {
    switch (d) {
        case SD_IP4:
            return AF_INET;
        case SD_IP6:
            return AF_INET6;
        case SD_UNIX:
            return AF_UNIX;
        default:
            throw std::runtime_error("unexpected asocket domain");
    }
}

static int socktype(t_socket_type t) {
    switch (t) {
        case ST_TCP:
        case ST_STREAM:
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

ASocket::ASocket(t_socket_domain sdomain, t_socket_type stype) : dying(false) {
    int d = sockdomain(sdomain);
    int t = socktype(stype);

    // DOUT() << "create asocket for: " << d << ", " << t << "..." << std::endl;
    t_fd sock = socket(d, t, 0);
    if (sock == -1) {
        throw std::runtime_error("failed to initialize asocket");
    }
    // DOUT() << "created asocket." << std::endl;
    fd     = sock;
    domain = sdomain;
    type   = stype;

    int yes = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&yes, sizeof(yes));
}

ASocket::ASocket(t_fd sock_fd, t_socket_domain sdomain, t_socket_type stype) : fd(sock_fd), dying(false) {
    domain = sdomain;
    type   = stype;
}

ASocket::ASocket(const ASocket &other) {
    *this = other;
}

ASocket &ASocket::operator=(const ASocket &rhs) {
    if (this != &rhs) {
        fd     = rhs.fd;
        domain = rhs.domain;
        type   = rhs.type;
        dying  = rhs.dying;
    }
    return *this;
}

ASocket::~ASocket() {
    destroy();
}

void ASocket::set_nonblock() {
    int rv;
    rv = fcntl(fd, F_SETFL, O_NONBLOCK);
    if (rv < 0) {
        throw std::runtime_error("failed to fcntl");
    }
}

int ASocket::get_fd() const {
    return fd;
}

t_socket_domain ASocket::get_domain() const {
    return domain;
}

t_socket_type ASocket::get_type() const {
    return type;
}

t_port ASocket::get_port() const {
    return port;
}

bool ASocket::get_dying() const {
    return dying;
}

void ASocket::destroy() {
    close(fd);
}
