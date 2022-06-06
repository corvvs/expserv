#ifndef ISOCKETLIKE_HPP
# define ISOCKETLIKE_HPP
# include "ipanopticon.hpp"

class ISocketLike {
public:
    virtual         ~ISocketLike() {};
    virtual t_fd    get_fd() const = 0;
    virtual void    notify(IObserver& loop) = 0;
};

#endif
