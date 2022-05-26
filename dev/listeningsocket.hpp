#ifndef LISTENINGSOCKET_HPP
# define LISTENINGSOCKET_HPP
# include "connectedsocket.hpp"

class ListeningSocket: public Socket {
private:
    ListeningSocket();
    ListeningSocket(
        SocketDomain sdomain,
        SocketType stype
    );

public:
    ListeningSocket(const ListeningSocket& other);
    ListeningSocket& operator=(const ListeningSocket& rhs);

    static ListeningSocket    bind(
        SocketDomain sdomain,
        SocketType stype,
        t_port port
    );
    void            listen(int backlog);
    ConnectedSocket accept();
};

#endif
