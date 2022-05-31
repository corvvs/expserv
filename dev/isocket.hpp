#ifndef ISOCKET_HPP
# define ISOCKET_HPP
# include "ipanopticon.hpp"

class ISocket {
public:
    virtual         ~ISocket() {};
    virtual t_fd    get_fd() const = 0;
    virtual void    notify(IPanopticon& loop) = 0;
};

#endif
