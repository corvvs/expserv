#include "eventkqueueloop.hpp"


const int EventKqueueLoop::nev = 10;

EventKqueueLoop::EventKqueueLoop() {
    evlist.resize(10);
    t_kqueue q = kqueue();
    if (q < 0) {
        throw std::runtime_error("failed to create kqueue");
    }
    kq = q;
}

EventKqueueLoop::~EventKqueueLoop() {
    for (socket_map::iterator it = sockmap.begin(); it != sockmap.end(); it++) {
        delete it->second;
    }
}

EventKqueueLoop::t_kfilter  EventKqueueLoop::filter(SocketHolderMapType t) {
    switch (t) {
    case SHMT_READ:
        return EVFILT_READ;
    case SHMT_WRITE:
        return EVFILT_WRITE;
    case SHMT_EXCEPTION:
        return EVFILT_EXCEPT;
    default:
        return 0;
    }
}

void    EventKqueueLoop::loop() {
    while (1) {
        update();
        int count = kevent(kq, NULL, 0, &*evlist.begin(), nev, NULL);
        std::cout << "[S]loop: " << count << std::endl;
        for (int i = 0; i < count; i++) {
            int fd = evlist[i].ident;
            ISocketLike* sock = sockmap[fd];
            sock->notify(*this);
        }
        std::cout << "[S]loop: end" << std::endl;
    }
}

void    EventKqueueLoop::preserve(ISocketLike* socket, SocketHolderMapType from, SocketHolderMapType to) {
    SocketPreservation  pre = {socket, from, to};
    upqueue.push_back(pre);
}

// 次のselectの前に, このソケットを監視対象から除外する
// (その際ソケットはdeleteされる)
void    EventKqueueLoop::preserve_clear(ISocketLike* socket, SocketHolderMapType from) {
    preserve(socket, from, SHMT_NONE);
}

// 次のselectの前に, このソケットを監視対象に追加する
void    EventKqueueLoop::preserve_set(ISocketLike* socket, SocketHolderMapType to) {
    preserve(socket, SHMT_NONE, to);
}

// 次のselectの前に, このソケットの監視方法を変更する
void    EventKqueueLoop::preserve_move(ISocketLike* socket, SocketHolderMapType from, SocketHolderMapType to) {
    preserve(socket, from, to);
}

void    EventKqueueLoop::update() {
    std::vector< t_kevent > changelist;
    changelist.reserve(upqueue.size());
    if (upqueue.size() == 0) { return; }
    int n = 0;
    for (update_queue::iterator it = upqueue.begin(); it != upqueue.end(); it++) {
        t_kevent    ke;
        ISocketLike*    sock = it->sock;
        t_fd        fd = sock->get_fd();
        if (it->to == SHMT_NONE) {
            // std::cout << "clearing " << sock->get_fd() << std::endl;
            sockmap.erase(fd);
            delete sock;
        } else {
            // std::cout << "updating " << sock->get_fd() << std::endl;
            changelist.push_back(ke);
            EV_SET(&*changelist.rbegin(), sock->get_fd(), filter(it->to), EV_ADD, 0, 0, NULL);
            sockmap[fd] = sock;
            n++;
        }
    }
    // std::cout << changelist.size() << std::endl;
    if (n > 0) {
        errno = 0;
        int count = kevent(kq, &*changelist.begin(), changelist.size(), NULL, 0, NULL);
        // std::cout << count << std::endl;
        if (errno) {
            std::cout << "errno: " << errno  << ", " << changelist.size() << ", " << n << ", " << count << std::endl;
        }
    }
    upqueue.clear();
}
