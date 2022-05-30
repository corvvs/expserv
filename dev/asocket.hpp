#ifndef ASOCKET_HPP
# define ASOCKET_HPP
# include "test_common.hpp"
# include "socket_type.hpp"

class EventLoop;

class ASocket {
protected:
    t_fd            fd;
    SocketDomain    domain;
    SocketType      type;
    t_port          port;
    bool            holding;

    int             run_counter;

private:
    // コンストラクタの直接呼び出しは禁止
    // Socketはfactoryメソッドbind, connectおよびインスタンスメソッドacceptによってのみ生成される

    // デフォルトコンストラクタは使用禁止(呼び出すと例外を投げる)
    ASocket();

protected:
    ASocket(
        SocketDomain sdomain,
        SocketType stype
    );

    ASocket(
        int fd,
        SocketDomain sdomain,
        SocketType stype
    );

    ASocket(const ASocket& other);
    void    destroy();

public:
    virtual ~ASocket();
    ASocket& operator=(const ASocket& rhs);

    virtual int     get_fd() const = 0;
    SocketDomain    get_domain() const;
    SocketType      get_type() const;
    t_port          get_port() const;

    virtual void    notify(EventLoop& loop) = 0;
};

#endif
