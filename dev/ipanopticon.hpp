#ifndef IPANOPTICON_HPP
# define IPANOPTICON_HPP
# include "isocketlike.hpp"

class IPanopticon {
public:
    virtual         ~IPanopticon() {};

    virtual void    loop() = 0;
    virtual void    preserve_clear(ISocketLike* socket, SocketHolderMapType from) = 0;
    virtual void    preserve_set(ISocketLike* socket, SocketHolderMapType to) = 0;
    virtual void    preserve_move(ISocketLike* socket, SocketHolderMapType from, SocketHolderMapType to) = 0;
};

#endif
