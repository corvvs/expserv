#ifndef IPANOPTICON_HPP
# define IPANOPTICON_HPP
# include "isocketlike.hpp"

class ISocketLike;

// [ソケット監視者インターフェース]
// ソケット(ISocketLike)の状態変化を監視し, 変化があったソケットに通知を出すクラス.
class IObserver {
public:
    virtual         ~IObserver() {};

    virtual void    sweep() = 0;
    virtual void    preserve_clear(ISocketLike* socket, t_socket_operation from) = 0;
    virtual void    preserve_set(ISocketLike* socket, t_socket_operation to) = 0;
    virtual void    preserve_move(ISocketLike* socket, t_socket_operation from, t_socket_operation to) = 0;
};

#endif
