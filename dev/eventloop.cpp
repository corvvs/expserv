#include "eventloop.hpp"

void    destroy_all(EventLoop::socket_map &m) {
    for (EventLoop::socket_map::iterator  it = m.begin(); it != m.end(); it++) {
        delete it->second;
    }
}

EventLoop::EventLoop() {
}

EventLoop::~EventLoop() {
    destroy_all(read_map);
    destroy_all(write_map);
    destroy_all(exception_map);
}

void    EventLoop::watch(Socket* socket, SocketHolderMapType map_type) {
    switch (map_type) {
        case SHMT_READ:
            read_map[socket->get_fd()] = socket;
            std::cout << "WATCH READ " << socket->get_fd() << std::endl;
            break;
        case SHMT_WRITE:
            write_map[socket->get_fd()] = socket;
            std::cout << "WATCH WRITE " << socket->get_fd() << std::endl;
            break;
        case SHMT_EXCEPTION:
            exception_map[socket->get_fd()] = socket;
            std::cout << "WATCH EXCEPTION " << socket->get_fd() << std::endl;
            break;
        default:
            throw std::runtime_error("unexpected map_type");
    }
}

void    EventLoop::unwatch(Socket* socket, SocketHolderMapType map_type) {
    switch (map_type) {
        case SHMT_READ:
            read_map.erase(socket->get_fd());
            std::cout << "UNWATCH READ " << socket->get_fd() << std::endl;
            break;
        case SHMT_WRITE:
            write_map.erase(socket->get_fd());
            std::cout << "UNWATCH WRITE " << socket->get_fd() << std::endl;
            break;
        case SHMT_EXCEPTION:
            exception_map.erase(socket->get_fd());
            std::cout << "UNWATCH EXCEPTION " << socket->get_fd() << std::endl;
            break;
        default:
            throw std::runtime_error("unexpected map_type");
    }
}

void    EventLoop::prepare_fd_set(socket_map& sockmap, fd_set *sockset) {
    FD_ZERO(sockset);
    for (socket_map::iterator it = sockmap.begin(); it != sockmap.end(); it++) {
        FD_SET(it->first, sockset);
    }
}

void    EventLoop::scan_fd_set(socket_map& sockmap, fd_set *sockset) {
    for (EventLoop::socket_map::iterator it = sockmap.begin(); it != sockmap.end(); it++) {
        int fd = it->first;
        std::cout << "fd is " << fd << std::endl;
        if (FD_ISSET(fd, sockset)) {
            // ready for read
            std::cout << "fd is ready: " << fd << std::endl;
            it->second->run(*this);
        }
    }
}

void    EventLoop::loop() {
    fd_set  read_set;
    fd_set  write_set;
    fd_set  exception_set;

    prepare_fd_set(read_map, &read_set);
    prepare_fd_set(write_map, &write_set);
    prepare_fd_set(exception_map, &exception_set);

    struct timeval tv;
    tv.tv_sec = 10;
    tv.tv_usec = 0;

    int max_fd = -1;
    if (!read_map.empty()) {
        max_fd = std::max(max_fd, read_map.rbegin()->first);
    }
    if (!write_map.empty()) {
        max_fd = std::max(max_fd, write_map.rbegin()->first);
    }
    if (!exception_map.empty()) {
        max_fd = std::max(max_fd, exception_map.rbegin()->first);
    }

    int count = select( max_fd + 1, &read_set, &write_set, &exception_set, &tv);
    if (count < 0) {
        throw std::runtime_error("select error");
    }
    scan_fd_set(read_map, &read_set);
    scan_fd_set(write_map, &write_set);
    scan_fd_set(exception_map, &exception_set);
    update();
}

void    EventLoop::preserve(Socket* socket, SocketHolderMapType from, SocketHolderMapType to) {
    SocketPreservation  pre = {
        .sock = socket,
        .from = from,
        .to = to,
    };
    up_queue.push_back(pre);
}

void    EventLoop::preserve_clear(Socket* socket, SocketHolderMapType from) {
    preserve(socket, from, SHMT_NONE);
}

void    EventLoop::preserve_set(Socket* socket, SocketHolderMapType to) {
    preserve(socket, SHMT_NONE, to);
}

void    EventLoop::preserve_move(Socket* socket, SocketHolderMapType from, SocketHolderMapType to) {
    preserve(socket, from, to);
}

void    EventLoop::update() {
    for (EventLoop::update_queue::iterator it = up_queue.begin(); it != up_queue.end(); it++) {
        if (it->from != SHMT_NONE) {
            unwatch(it->sock, it->from);
        }
        if (it->to != SHMT_NONE) {
            watch(it->sock, it->to);
        } else {
            delete it->sock;
        }
    }
    up_queue.clear();
}
