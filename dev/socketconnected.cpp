#include "socketconnected.hpp"
#include "socketlistening.hpp"
#define N 1024

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

SocketConnected::SocketConnected(const SocketConnected& other): ASocket(other) {}

SocketConnected::SocketConnected(
        t_socket_domain sdomain,
        t_socket_type stype
): ASocket(sdomain, stype) {}

SocketConnected::SocketConnected(
    t_fd accepted_fd,
    SocketListening& listening_socket
): ASocket(accepted_fd, listening_socket.get_domain(), listening_socket.get_type()) {
    port = listening_socket.get_port();
}

SocketConnected& SocketConnected::operator=(const SocketConnected& rhs) {
    static_cast<ASocket&>(*this) = static_cast<const ASocket&>(rhs);
    return *this;
}

SocketConnected*    SocketConnected::connect(
    t_socket_domain sdomain,
    t_socket_type stype,
    t_port port
) {

    SocketConnected* sock = new SocketConnected(sdomain, stype);
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

ssize_t SocketConnected::send(const void *buffer, size_t len, int flags) {
    return ::send(get_fd(), buffer, len, flags);
}

ssize_t SocketConnected::receive(void *buffer, size_t len, int flags) {
    return ::recv(get_fd(), buffer, len, flags);
}

void    SocketConnected::notify(IPanopticon& loop) {
    std::cout << "[S]" << fd << " rc: " << run_counter << std::endl;
    if (dying) { return; }
    switch (run_counter) {
        case 0: {
            char buf[N];
            ssize_t receipt = receive(&buf, N, 0);
            if (receipt <= 0) {
                std::cout << "[S]" << receipt_str;
                std::cout << " fd: " << get_fd();
                std::cout << " port: " << get_port();
                std::cout << std::endl;
                receipt_str.clear();

                // loop.preserve_clear(this, SHMT_WRITE);
                loop.preserve_move(this, SHMT_READ, SHMT_WRITE);
                run_counter++;
                return;
            }
            receipt_str += std::string(buf, receipt);
            // std::cout << "S[fd: " << get_fd();
            // std::cout << " port: " << get_port();
            // std::cout << "] " << std::string(buf, receipt);
            // std::cout << std::endl;
            return;
        }
        case 1: {
            // レスポンス返す処理
            const std::string rs = "hello back";
            send(rs.c_str(), rs.length(), 0);
            loop.preserve_clear(this, SHMT_WRITE);
            dying = true;
            run_counter++;
            return;
        }
        default:
            std::cout << "FAIL: " << fd << std::endl;
            throw std::runtime_error("????");
    }
}

int             SocketConnected::get_fd() const {
    return fd;
}
