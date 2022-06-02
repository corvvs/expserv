#ifndef CHANNEL_HPP
# define CHANNEL_HPP
# include <map>
# include <utility>
# include "listeningsocket.hpp"
# include "isocketlike.hpp"
# include "ipanopticon.hpp"

class Channel: public ISocketLike {
public:
    typedef std::pair<SocketDomain, t_port> t_channel_id;

private:
    ListeningSocket*    sock;

    // 呼び出し禁止
    Channel();
    // 直接呼び出し禁止
    Channel(SocketDomain sdomain, SocketType stype, t_port port);

public:
    ~Channel();

    static Channel* listen(SocketDomain sdomain, SocketType stype, t_port port);

    t_fd    get_fd() const;
    void    notify(IPanopticon& loop);
};

#endif
