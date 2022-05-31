#ifndef IPANOPTICON_HPP
# define IPANOPTICON_HPP
# include "isocket.hpp"

class IPanopticon {
public:
    virtual         ~IPanopticon() {};

    virtual void    listen(
        SocketDomain sdomain,
        SocketType stype,
        t_port port
    ) = 0;
    virtual void    loop() = 0;
    virtual void    preserve_clear(ISocket* socket, SocketHolderMapType from) = 0;
    virtual void    preserve_set(ISocket* socket, SocketHolderMapType to) = 0;
    virtual void    preserve_move(ISocket* socket, SocketHolderMapType from, SocketHolderMapType to) = 0;
};

#endif
