#ifndef ISOCKETLIKE_HPP
# define ISOCKETLIKE_HPP
# include "iobserver.hpp"

class IObserver;

// [ソケットインターフェース]
// ソケット監視者(IObserver)の監視対象となりうるクラス.
// 実際のソケットとは限らない.
class ISocketLike {
public:
    virtual         ~ISocketLike() {};
    virtual t_fd    get_fd() const = 0;
    virtual void    notify(IObserver& loop) = 0;
};

#endif
