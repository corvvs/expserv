#include "listeningsocket.hpp"

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

static void cpp_bzero(void *mem, size_t n) {
    for (size_t i = 0; i < n; i++) {
        ((unsigned char*)mem)[i] = 0;
    }
}

ListeningSocket::ListeningSocket(
    SocketDomain sdomain,
    SocketType stype
): Socket(sdomain, stype) {}

ListeningSocket& ListeningSocket::operator=(const ListeningSocket& rhs) {
    static_cast<Socket&>(*this) = static_cast<const Socket&>(rhs);
    return *this;
}

ListeningSocket*    ListeningSocket::bind(
    SocketDomain sdomain,
    SocketType stype,
    t_port port
) {

    ListeningSocket* sock = new ListeningSocket(sdomain, stype);
    sock->waitAccept();
    int fd = sock->get_fd();

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

void    ListeningSocket::listen(int backlog) {
    DOUT() << "making socket listening in backlog: " << backlog << std::endl;
    if (::listen(fd, backlog) == -1) {
        throw std::runtime_error("failed to listen");
    }
    DOUT() << "now listening..." << std::endl;
}

void            ListeningSocket::waitAccept() {
    int rv;
    rv = fcntl(fd, F_SETFL, O_NONBLOCK);
    std::cout << "rv: " << rv << ", " << errno << std::endl;
    std::cout << strerror(errno) << std::endl;
    // rv = ::accept(fd, NULL, NULL);
    // std::cout << "rv: " << rv << ", " << errno << std::endl;
    // std::cout << strerror(errno) << std::endl;
}


ConnectedSocket*    ListeningSocket::accept() {
    int accepted_fd = ::accept(fd, NULL, NULL);
    DOUT() << "accepted" << std::endl;
    if (accepted_fd < 0) {
        throw std::runtime_error("failed to accept");
    }
    return new ConnectedSocket(accepted_fd, *this);
}

void            ListeningSocket::run(EventLoop& loop) {
    std::cout << "run_counter is " << run_counter << std::endl;
    switch (run_counter) {
        case 0: {
            ConnectedSocket* accepted = accept();
            loop.preserve_set(accepted, SHMT_READ);
            return;
        }
    }
}
