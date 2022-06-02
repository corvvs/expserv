#ifndef CONNECTION_HPP
# define CONNECTION_HPP
# include "socketconnected.hpp"
# include "isocketlike.hpp"
# include "ipanopticon.hpp"

class Connection {
public:
    ~Connection();

    t_fd    get_fd() const;
    void    notify(IPanopticon& loop);
};

#endif;
