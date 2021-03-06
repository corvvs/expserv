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
        ((unsigned char *)mem)[i] = 0;
    }
}

SocketListening::SocketListening(t_socket_domain sdomain, t_socket_type stype) : ASocket(sdomain, stype) {}

SocketListening &SocketListening::operator=(const SocketListening &rhs) {
    static_cast<ASocket &>(*this) = static_cast<const ASocket &>(rhs);
    return *this;
}

SocketListening *SocketListening::bind(t_socket_domain sdomain, t_socket_type stype, t_port port) {
    SocketListening *sock = new SocketListening(sdomain, stype);
    sock->set_nonblock();
    t_fd fd = sock->fd;

    struct sockaddr_in sa;
    cpp_bzero(&sa, sizeof(sa));
    int d              = sockdomain(sdomain);
    sa.sin_family      = d;
    sa.sin_port        = htons(port);
    sa.sin_addr.s_addr = htonl(INADDR_ANY);
    DXOUT("binding asocket for: " << port << ", " << sa.sin_addr.s_addr << "...");
    if (::bind(fd, (struct sockaddr *)&sa, sizeof(struct sockaddr_in)) == -1) {
        std::cerr << strerror(errno) << std::endl;
        throw std::runtime_error("failed to bind a asocket");
    }
    DXOUT("bound asocket.");
    sock->port = port;
    return sock;
}

void SocketListening::listen(int backlog) {
    // DOUT() << "making asocket listening in backlog: " << backlog << std::endl;
    if (::listen(fd, backlog) == -1) {
        throw std::runtime_error("failed to listen");
    }
    DXOUT("now listening...");
}

SocketConnected *SocketListening::accept() {
    t_fd accepted_fd = ::accept(fd, NULL, NULL);

    if (accepted_fd < 0) {
        if (errno == EWOULDBLOCK) {
            // これ以上 acceptできない
            return NULL;
        }
        throw std::runtime_error("failed to accept");
    }
    return SocketConnected::wrap(accepted_fd, *this);
}
