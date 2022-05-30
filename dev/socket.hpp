#ifndef SOCKET_HPP
# define SOCKET_HPP
# include <iostream>
# include <string>
# include <exception>
# include <stdexcept>
# include <sys/types.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <unistd.h>
# include <netdb.h>
# include "test_common.hpp"
# include "eventloop.hpp"


enum SocketDomain {
    SD_IP4,
    SD_IP6
};

enum SocketType {
    ST_TCP,
    ST_UDP
};

typedef uint16_t    t_port;
typedef uint32_t    t_addressv4;

class EventLoop;

class Socket {
protected:
    int             fd;
    SocketDomain    domain;
    SocketType      type;
    bool            holding;

    int             run_counter;

private:
    // コンストラクタの直接呼び出しは禁止
    // Socketはfactoryメソッドbind, connectおよびインスタンスメソッドacceptによってのみ生成される

    // デフォルトコンストラクタは使用禁止(呼び出すと例外を投げる)
    Socket();

protected:
    Socket(
        SocketDomain sdomain,
        SocketType stype
    );

    Socket(
        int fd,
        SocketDomain sdomain,
        SocketType stype
    );

    Socket(const Socket& other);
    void    destroy();

public:
    virtual ~Socket();
    Socket& operator=(const Socket& rhs);

    int             get_fd() const;
    SocketDomain    get_domain() const;
    SocketType      get_type() const;

    virtual void    run(EventLoop& loop) = 0;
};

#endif
