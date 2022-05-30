#ifndef ISOCKET_HPP
# define ISOCKET_HPP

class EventLoop;

class ISocket {
public:
    virtual         ~ISocket() {};
    virtual int     get_fd() const = 0;
    virtual void    notify(EventLoop& loop) = 0;
};

#endif
