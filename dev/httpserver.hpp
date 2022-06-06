#ifndef HTTPSERVER_HPP
# define HTTPSERVER_HPP
# include <map>
# include "channel.hpp"
# include "isocketlike.hpp"

template <
    class TObserver // * TObserver must conforms to IObserver *
>
class HTTPServer {
public:
    typedef std::map<Channel::t_channel_id, Channel*> channel_map;

private:
    channel_map channels;
    TObserver*  observer;

public:
    HTTPServer(): observer(new TObserver()) {

    }

    ~HTTPServer() {
        for (channel_map::iterator it = channels.begin(); it != channels.end(); it++) {
            delete it->second;
        }
        delete observer;
    }

    // ほんとはconfに基づいてやる
    void    listen(
        t_socket_domain sdomain,
        t_socket_type stype,
        t_port port
    ) {
        Channel *ch = Channel::listen(sdomain, stype, port);
        channels[ch->get_id()] = ch;
        observer->preserve_set(ch, SHMT_READ);
    }

    TObserver&  get_observer() const {
        return *observer;
    }

    void        loop() {
        observer->loop();
    }

};

#endif
