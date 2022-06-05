#ifndef CONNECTION_HPP
# define CONNECTION_HPP
# include "socketlistening.hpp"
# include "socketconnected.hpp"
# include "isocketlike.hpp"
# include "ipanopticon.hpp"
# include "requesthttp.hpp"
# include <string>
# include <iostream>

enum t_connection_phase {
    CONNECTION_NEUTRAL,
    CONNECTION_RECEIVING,
    CONNECTION_RESPONDING,
    CONNECTION_ERROR_RESPONDING
};

class Connection: public ISocketLike {
private:
    t_connection_phase  phase;
    bool                dying;
    std::string         receipt_str;

    SocketConnected*    sock;

    RequestHTTP*        current_req;

    // 直接呼び出し禁止
    Connection();


public:
    Connection(SocketListening* listening);
    ~Connection();

    t_fd    get_fd() const;
    void    notify(IPanopticon& loop);
};

#endif
