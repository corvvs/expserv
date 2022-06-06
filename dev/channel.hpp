#ifndef CHANNEL_HPP
# define CHANNEL_HPP
# include <map>
# include <utility>
# include "socketlistening.hpp"
# include "isocketlike.hpp"
# include "ipanopticon.hpp"

class Conenction;

class Channel: public ISocketLike {
public:
    // Channelを識別するためのID
    // プロトコルファミリーとポートの組
    typedef std::pair<t_socket_domain, t_port> t_channel_id;

private:
    SocketListening*    sock;

    // 呼び出し禁止
    Channel();

    // 直接呼び出し禁止
    Channel(t_socket_domain sdomain, t_socket_type stype, t_port port);

public:
    friend class Connection;

    ~Channel();

    static Channel* listen(t_socket_domain sdomain, t_socket_type stype, t_port port);

    t_fd            get_fd() const;
    void            notify(IObserver& loop);

    t_channel_id    get_id() const;
};

#endif
