#include "connection.hpp"
#include "channel.hpp"

Channel::Channel() {
    throw std::runtime_error("forbidden");
}

Channel::Channel(
    t_socket_domain sdomain,
    t_socket_type stype,
    t_port port
): sock(SocketListening::bind(sdomain, stype, port)) {
    sock->listen(128);
}

Channel::~Channel() {
    delete sock;
}

t_fd    Channel::get_fd() const {
    return sock->get_fd();
}

void    Channel::notify(IObserver& loop) {
    // Channelがnotifyを受ける
    // -> accept ready
    // -> Connectionを生成してread監視させる
    try {
        loop.preserve_set(new Connection(sock->accept()), SHMT_READ);
    } catch (...) {
        DSOUT() << "failed to accept socket: fd: " << sock->get_fd() << std::endl;
    }
}

Channel::t_channel_id   Channel::get_id() const {
    return Channel::t_channel_id(sock->get_domain(), sock->get_port());
}
