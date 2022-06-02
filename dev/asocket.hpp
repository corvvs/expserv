#ifndef ASOCKET_HPP
# define ASOCKET_HPP
# include "test_common.hpp"
# include "socket_type.hpp"

class IPanopticon;

class ASocket {
protected:
    t_fd            fd;
    t_socket_domain    domain;
    t_socket_type      type;
    t_port          port;
    bool            dying;

private:
    // コンストラクタの直接呼び出しは禁止
    // Socketはfactoryメソッドbind, connectおよびインスタンスメソッドacceptによってのみ生成される

    // デフォルトコンストラクタは使用禁止(呼び出すと例外を投げる)
    ASocket();

protected:
    ASocket(
        t_socket_domain sdomain,
        t_socket_type stype
    );

    ASocket(
        int fd,
        t_socket_domain sdomain,
        t_socket_type stype
    );

    ASocket(const ASocket& other);
    void    destroy();

public:
    virtual ~ASocket();
    ASocket& operator=(const ASocket& rhs);

    virtual int     get_fd() const = 0;
    t_socket_domain    get_domain() const;
    t_socket_type      get_type() const;
    t_port          get_port() const;
    bool            get_dying() const;
};

#endif
