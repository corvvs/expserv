#ifndef CONNECTION_HPP
# define CONNECTION_HPP
# include "socketlistening.hpp"
# include "socketconnected.hpp"
# include "isocketlike.hpp"
# include "ipanopticon.hpp"
# include <string>

class Connection: public ISocketLike {
private:
    int                 run_counter;
    bool                dying;
    std::string         receipt_str;

    SocketConnected*    sock;

    // 直接呼び出し禁止
    Connection();


public:
    Connection(SocketListening* listening);
    ~Connection();

    t_fd    get_fd() const;
    void    notify(IPanopticon& loop);
};

#endif
