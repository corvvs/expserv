#ifndef LISTENINGSOCKET_HPP
# define LISTENINGSOCKET_HPP
# include "asocket.hpp"
# include "isocket.hpp"
# include "eventloop.hpp"
# include "connectedsocket.hpp"

class ConnectedSocket;

class ListeningSocket: public ASocket, public ISocket {
private:
    ListeningSocket();
    ListeningSocket(
        SocketDomain sdomain,
        SocketType stype
    );

public:
    ListeningSocket(const ListeningSocket& other);
    ListeningSocket& operator=(const ListeningSocket& rhs);

    static ListeningSocket* bind(
        SocketDomain sdomain,
        SocketType stype,
        t_port port
    );
    void                listen(int backlog);
    void                waitAccept();
    ConnectedSocket*    accept();
    int get_fd() const;

    void    notify(EventLoop& loop);
};

#endif
