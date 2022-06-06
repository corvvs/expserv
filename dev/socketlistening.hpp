#ifndef LISTENINGSOCKET_HPP
# define LISTENINGSOCKET_HPP
# include "asocket.hpp"
# include "isocketlike.hpp"
# include "ipanopticon.hpp"
# include "socketconnected.hpp"

class SocketConnected;

class SocketListening: public ASocket {
private:
    SocketListening();
    SocketListening(
        t_socket_domain sdomain,
        t_socket_type stype
    );

public:
    SocketListening(const SocketListening& other);
    SocketListening& operator=(const SocketListening& rhs);

    static SocketListening* bind(
        t_socket_domain sdomain,
        t_socket_type stype,
        t_port port
    );
    void                listen(int backlog);
    void                waitAccept();
    SocketConnected*    accept();
    t_fd                get_fd() const;

    void    notify(IObserver& loop);
};

#endif
