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

Channel* Channel::listen(
    t_socket_domain sdomain,
    t_socket_type stype,
    t_port port
) {
    return new Channel(sdomain, stype, port);
}

t_fd    Channel::get_fd() const {
    return sock->get_fd();
}


void    Channel::notify(IPanopticon& loop) {
    SocketConnected* accepted = sock->accept();
    loop.preserve_set(accepted, SHMT_READ);
}
