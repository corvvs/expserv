#ifndef CONNECTEDSOCKET_HPP
# define CONNECTEDSOCKET_HPP
# include "asocket.hpp"

class SocketListening;

class SocketConnected: public ASocket {
private:
    // 使用禁止
    SocketConnected();

    // クライアント用コンストラクタ
    SocketConnected(
        t_socket_domain sdomain,
        t_socket_type stype
    );
    // サーバ用コンストラクタ
    // accept によって生成される
    SocketConnected(
        t_fd fd,
        SocketListening& listening
    );

public:
    SocketConnected(const SocketConnected& other);
    SocketConnected& operator=(const SocketConnected& rhs);

    // クライアント用connect関数ラッパ
    static SocketConnected  *connect(
        t_socket_domain sdomain,
        t_socket_type stype,
        t_port port
    );

    // サーバ用factory関数
    static SocketConnected  *wrap(t_fd fd, SocketListening& listening);

    ssize_t send(const void *buffer, size_t len, int flags);
    ssize_t receive(void *buffer, size_t len, int flags);
};

#endif
