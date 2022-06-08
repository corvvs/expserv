#ifndef ISOCKETLIKE_HPP
# define ISOCKETLIKE_HPP
# include "types.hpp"
# include "iobserver.hpp"

class IObserver;

// [ソケットライクインターフェース]
// [責務]
// - ある1つのソケットに紐づいていること
// - ソケット監視者からの通知を受け取り, しかるべき処理を行うこと
class ISocketLike {
public:
    virtual         ~ISocketLike() {};
    // 紐づいているソケットのfdを返す
    virtual t_fd    get_fd() const = 0;
    // ソケット監視者からの通知を受け取る
    virtual void    notify(IObserver& observer) = 0;
    // タイムアウトが疑われる時の処理
    virtual void    timeout(IObserver& observer, t_time_epoch_ms epoch) = 0;
};

#endif
