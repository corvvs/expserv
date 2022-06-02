#ifndef EVENTPOLLLOOP_HPP
# define EVENTPOLLLOOP_HPP
# include "asocket.hpp"
# include "isocketlike.hpp"
# include "socketlistening.hpp"
# include <map>
# include <set>
# include <vector>
# include <poll.h>
# include <sys/select.h>
# include <sys/time.h>
# include <sys/types.h>
# include <errno.h>
# include <unistd.h>
# include "socket_type.hpp"
# include "ipanopticon.hpp"

class ISocketLike;

typedef short t_poll_eventmask;

class EventPollLoop: public IPanopticon {
    private:
        typedef std::vector<pollfd>                 fd_vector;
        typedef std::map<t_fd, ISocketLike*>            socket_map;
        typedef std::map<t_fd, int>                 index_map;
        typedef std::set<int>                      gap_set;
        typedef std::vector< t_socket_reservation >   update_queue;

        fd_vector                                   fds;
        socket_map                                  sockmap;
        index_map                                   indexmap;
        gap_set                                     gapset;
        int                                         nfds;

        update_queue                                clearqueue;
        update_queue                                movequeue;
        update_queue                                setqueue;

        t_poll_eventmask    mask(t_socket_operation t);

        void    preserve(ISocketLike* socket, t_socket_operation from, t_socket_operation to);
        void    update();

    public:
        EventPollLoop();
        ~EventPollLoop();

        void    loop();
        void    preserve_clear(ISocketLike* socket, t_socket_operation from);
        void    preserve_set(ISocketLike* socket, t_socket_operation to);
        void    preserve_move(ISocketLike* socket, t_socket_operation from, t_socket_operation to);
};

#endif
