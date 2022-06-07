#ifndef ISOCKETLIKE_HPP
# define ISOCKETLIKE_HPP
# include "iobserver.hpp"

class IObserver;

// [ソケットライクインターフェース]
// [責務]
// - ある1つのソケットに紐づいていること
// - ソケット監視者からの通知を受け取り, しかるべき処理を行うこと
class ISocketLike {
public:
    virtual         ~ISocketLike() {};
    virtual t_fd    get_fd() const = 0;
    virtual void    notify(IObserver& loop) = 0;
};

#endif
