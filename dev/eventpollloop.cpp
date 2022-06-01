#include "eventpollloop.hpp"

EventPollLoop::EventPollLoop(): nfds(0) {
}

EventPollLoop::~EventPollLoop() {
    for (socket_map::iterator it = sockmap.begin(); it != sockmap.end(); it++) {
        delete it->second;
    }
}

void    EventPollLoop::listen(
    SocketDomain sdomain,
    SocketType stype,
    t_port port
) {
    ListeningSocket* sock = ListeningSocket::bind(sdomain, stype, port);
    sock->listen(128);
    preserve_set(sock, SHMT_READ);
}

// イベントループ
void    EventPollLoop::loop() {
    while (1) {
        update();
        std::cout << "[S] polling count: " << nfds << std::endl;

        std::cout << "[S]";
        for (fd_vector::iterator it = fds.begin(); it != fds.end(); it++) {
            std::cout << " ";
            if (it->fd >= 0) {
                std::cout << it->fd;
            } else {
                std::cout << "xx";
            }
            std::cout << ":" << it->events;
        }
        std::cout << std::endl;

        int count = poll(&*fds.begin(), fds.size(), 10 * 1000);
        // std::cout << "polled. " << count << std::endl;
        if (count < 0) {
            throw std::runtime_error("poll error");
        }
        for (socket_map::iterator it = sockmap.begin(); it != sockmap.end(); it++) {
            int i = indexmap[it->first];
            if (fds[i].fd >= 0 && fds[i].revents) {
                std::cout << "[S]FD-" << it->first << ": revents: " << fds[i].revents << std::endl;
                it->second->notify(*this);
            }
        }
    }
}

void    EventPollLoop::preserve(ISocket* socket, SocketHolderMapType from, SocketHolderMapType to) {
    SocketPreservation  pre = {socket, from, to};
    if (from != SHMT_NONE && to == SHMT_NONE) {
        clearqueue.push_back(pre);
    }
    if (from != SHMT_NONE && to != SHMT_NONE) {
        movequeue.push_back(pre);
    }
    if (from == SHMT_NONE && to != SHMT_NONE) {
        setqueue.push_back(pre);
    }
}

// このソケットを監視対象から除外する
// (その際ソケットはdeleteされる)
void    EventPollLoop::preserve_clear(ISocket* socket, SocketHolderMapType from) {
    preserve(socket, from, SHMT_NONE);
}

// このソケットを監視対象に追加する
void    EventPollLoop::preserve_set(ISocket* socket, SocketHolderMapType to) {
    preserve(socket, SHMT_NONE, to);
}

// このソケットの監視方法を変更する
void    EventPollLoop::preserve_move(ISocket* socket, SocketHolderMapType from, SocketHolderMapType to) {
    preserve(socket, from, to);
}

t_poll_eventmask    EventPollLoop::mask(SocketHolderMapType t) {
    switch (t) {
    case SHMT_READ:
        return POLLIN;
    case SHMT_WRITE:
        return POLLOUT;
    case SHMT_EXCEPTION:
        return POLLPRI;
    default:
        return 0;
    }
}


// ソケットの監視状態変更予約を実施する
void    EventPollLoop::update() {
    EventPollLoop::update_queue::iterator   it;
    for (it = clearqueue.begin(); it != clearqueue.end(); it++) {
        ISocket* sock = it->sock;
        std::cout << "[S]FD-" << sock->get_fd() << ": clearing" << std::endl;
        int i = indexmap[sock->get_fd()];
        fds[i].fd = -1;
        sockmap.erase(sock->get_fd());
        indexmap.erase(sock->get_fd());
        gapset.insert(i);
        delete sock;
        nfds--;
    }
    for (it = movequeue.begin(); it != movequeue.end(); it++) {
        ISocket* sock = it->sock;
        std::cout << "[S]FD-" << sock->get_fd() << ": moving" << std::endl;
        int i = indexmap[sock->get_fd()];
        fds[i].events = mask(it->to);
    }
    for (it = setqueue.begin(); it != setqueue.end(); it++) {
        ISocket* sock = it->sock;
        std::cout << "[S]FD-" << sock->get_fd() << ": setting" << std::endl;
        int i;
        if (gapset.empty()) {
            pollfd p = {};
            p.fd = sock->get_fd();
            i = fds.size();
            fds.push_back(p);
        } else {
            i = *(gapset.begin());
            fds[i].fd = sock->get_fd();
            gapset.erase(gapset.begin());
        }
        fds[i].events = mask(it->to);
        sockmap[sock->get_fd()] = sock;
        indexmap[sock->get_fd()] = i;
        nfds++;
    }
    clearqueue.clear();
    movequeue.clear();
    setqueue.clear();
}
