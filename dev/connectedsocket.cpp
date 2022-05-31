#include "connectedsocket.hpp"
#include "listeningsocket.hpp"
#define N 1024

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

ConnectedSocket::ConnectedSocket(const ConnectedSocket& other): ASocket(other) {}

ConnectedSocket::ConnectedSocket(
        SocketDomain sdomain,
        SocketType stype
): ASocket(sdomain, stype) {}

ConnectedSocket::ConnectedSocket(
    t_fd accepted_fd,
    ListeningSocket& listening_socket
): ASocket(accepted_fd, listening_socket.get_domain(), listening_socket.get_type()) {
    port = listening_socket.get_port();
}

ConnectedSocket& ConnectedSocket::operator=(const ConnectedSocket& rhs) {
    static_cast<ASocket&>(*this) = static_cast<const ASocket&>(rhs);
    return *this;
}

ConnectedSocket*    ConnectedSocket::connect(
    SocketDomain sdomain,
    SocketType stype,
    t_port port
) {

    ConnectedSocket* sock = new ConnectedSocket(sdomain, stype);
    t_fd fd = sock->get_fd();

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
    if (::connect(fd, (struct sockaddr*) &sa, sizeof(struct sockaddr_in)) == -1) {
        throw std::runtime_error("failed to connect");
    }
    sock->port = port;
    return sock;
}

ssize_t ConnectedSocket::send(const void *buffer, size_t len, int flags) {
    return ::send(get_fd(), buffer, len, flags);
}

ssize_t ConnectedSocket::receive(void *buffer, size_t len, int flags) {
    return ::recv(get_fd(), buffer, len, flags);
}

void    ConnectedSocket::notify(IPanopticon& loop) {
    switch (run_counter) {
        case 0: {
            char buf[N];
            ssize_t receipt = receive(&buf, N, 0);
            if (receipt <= 0) {
                std::cout << receipt_str;
                std::cout << " fd: " << get_fd();
                std::cout << " port: " << get_port();
                std::cout << std::endl;
                receipt_str.clear();

                loop.preserve_clear(this, SHMT_READ);
                run_counter++;
                return;
            }
            receipt_str += std::string(buf, receipt);
            return;
        }
        case 1: {
            // レスポンス返す処理
        }
    }
}

int             ConnectedSocket::get_fd() const {
    return fd;
}
