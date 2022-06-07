#ifndef IPANOPTICON_HPP
# define IPANOPTICON_HPP
# include "isocketlike.hpp"

// [ソケット監視者インターフェース]
// [責務]
// - ソケットライクオブジェクト(ISocketLike)を保持すること
// - ソケットライクオブジェクトの状態変化を監視し, 変化があったら通知を出すこと
class IObserver {
public:
    virtual         ~IObserver() {};

    virtual void    loop() = 0;
    virtual void    reserve_clear(ISocketLike* socket, t_socket_operation from) = 0;
    virtual void    reserve_set(ISocketLike* socket, t_socket_operation to) = 0;
    virtual void    reserve_move(ISocketLike* socket, t_socket_operation from, t_socket_operation to) = 0;
};

#endif
