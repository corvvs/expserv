#ifndef CONNECTION_HPP
# define CONNECTION_HPP
# include "socketlistening.hpp"
# include "socketconnected.hpp"
# include "isocketlike.hpp"
# include "ipanopticon.hpp"
# include "requesthttp.hpp"
# include "responsehttp.hpp"
# include <string>
# include <iostream>

enum t_connection_phase {
    CONNECTION_NEUTRAL,
    CONNECTION_RECEIVING,
    CONNECTION_RESPONDING,
    CONNECTION_ERROR_RESPONDING
};

class Channel;
class Connection: public ISocketLike {
private:
    t_connection_phase  phase;
    bool                dying;

    Channel*            origin_channel;
    SocketConnected*    sock;

    RequestHTTP*        current_req;
    ResponseHTTP*       current_res;

    // 直接呼び出し禁止
    Connection();


public:
    Connection(Channel* origin);
    ~Connection();

    t_fd    get_fd() const;
    void    notify(IObserver& loop);
};

#endif
