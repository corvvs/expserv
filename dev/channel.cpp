#include "channel.hpp"

Channel::Channel() {
    throw std::runtime_error("forbidden");
}

Channel::Channel(
    SocketDomain sdomain,
    SocketType stype,
    t_port port
): sock(ListeningSocket::bind(sdomain, stype, port)) {
    sock->listen(128);
}

Channel::~Channel() {
    delete sock;
}

Channel* Channel::listen(
    SocketDomain sdomain,
    SocketType stype,
    t_port port
) {
    return new Channel(sdomain, stype, port);
}

t_fd    Channel::get_fd() const {
    return sock->get_fd();
}


void    Channel::notify(IPanopticon& loop) {
    ConnectedSocket* accepted = sock->accept();
    loop.preserve_set(accepted, SHMT_READ);
}
