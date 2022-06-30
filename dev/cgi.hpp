#ifndef CGI_HPP
#define CGI_HPP
#include "SocketUNIX.hpp"
#include "http.hpp"
#include "iobserver.hpp"
#include "irouter.hpp"
#include "isocketlike.hpp"
#include "requesthttp.hpp"
#include <map>

class CGI : public ISocketLike {
public:
    typedef HTTP::byte_string byte_string;
    typedef HTTP::light_string light_string;
    typedef byte_string::size_type size_type;
    typedef std::map<byte_string, byte_string> metavar_dict_type;

private:
    pid_t cgi_pid;
    SocketUNIX *sock;
    byte_string script_path_;
    byte_string query_string_;
    metavar_dict_type metavar_;
    size_type content_length_;

    CGI();
    CGI(pid_t cgi_pid,
        SocketUNIX *sock,
        byte_string &script_path,
        byte_string &query_string,
        metavar_dict_type &metavar,
        size_type content_length);

    static char **flatten_argv(const byte_string &script_path);
    static char **flatten_metavar(const metavar_dict_type &metavar);

public:
    ~CGI();

    static CGI *start_cgi(byte_string &script_path,
                          byte_string &query_string,
                          metavar_dict_type &metavar,
                          size_type content_length);

    t_fd get_fd() const;
    virtual void notify(IObserver &observer);
    virtual void timeout(IObserver &observer, t_time_epoch_ms epoch);
};

#endif
