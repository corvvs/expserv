#include "socketlistening.hpp"

static int sockdomain(t_socket_domain d) {
    switch (d) {
    case SD_IP4:
        return AF_INET;
    case SD_IP6:
        return AF_INET6;
    default:
        throw std::runtime_error("unexpected socket domain");
    }
}

static void cpp_bzero(void *mem, size_t n) {
    for (size_t i = 0; i < n; i++) {
        ((unsigned char*)mem)[i] = 0;
    }
}

SocketListening::SocketListening(
    t_socket_domain sdomain,
    t_socket_type stype
): ASocket(sdomain, stype) {}

SocketListening& SocketListening::operator=(const SocketListening& rhs) {
    static_cast<ASocket&>(*this) = static_cast<const ASocket&>(rhs);
    return *this;
}

SocketListening*    SocketListening::bind(
    t_socket_domain sdomain,
    t_socket_type stype,
    t_port port
) {

    SocketListening* sock = new SocketListening(sdomain, stype);
    sock->waitAccept();
    t_fd fd = sock->get_fd();

    struct sockaddr_in sa;
    cpp_bzero(&sa, sizeof(sa));
    int d = sockdomain(sdomain);
    sa.sin_family = d;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = htonl(INADDR_ANY);
    DOUT() << "binding asocket for: " << port << ", " << sa.sin_addr.s_addr << "..." << std::endl;
    if (::bind(fd, (struct sockaddr*) &sa, sizeof(struct sockaddr_in)) == -1) {
        throw std::runtime_error("failed to bind a asocket");
    }
    DOUT() << "bound asocket." << std::endl;
    sock->port = port;
    return sock;
}

void    SocketListening::listen(int backlog) {
    // DOUT() << "making asocket listening in backlog: " << backlog << std::endl;
    if (::listen(fd, backlog) == -1) {
        throw std::runtime_error("failed to listen");
    }
    DOUT() << "now listening..." << std::endl;
}

void            SocketListening::waitAccept() {
    int rv;
    rv = fcntl(fd, F_SETFL, O_NONBLOCK);
    if (rv < 0) {
        throw std::runtime_error("failed to fcntl");
    }
}


SocketConnected*    SocketListening::accept() {
    t_fd accepted_fd = ::accept(fd, NULL, NULL);
    // DOUT() << "accepted" << std::endl;
    if (accepted_fd < 0) {
        throw std::runtime_error("failed to accept");
    }
    return new SocketConnected(accepted_fd, *this);
}

int             SocketListening::get_fd() const {
    return fd;
}