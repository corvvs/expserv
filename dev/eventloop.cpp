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

void    EventLoop::watch(ISocket* socket, SocketHolderMapType map_type) {
    switch (map_type) {
        case SHMT_READ:
            read_map[socket->get_fd()] = socket;
            break;
        case SHMT_WRITE:
            write_map[socket->get_fd()] = socket;
            break;
        case SHMT_EXCEPTION:
            exception_map[socket->get_fd()] = socket;
            break;
        default:
            throw std::runtime_error("unexpected map_type");
    }
}

void    EventLoop::unwatch(ISocket* socket, SocketHolderMapType map_type) {
    switch (map_type) {
        case SHMT_READ:
            read_map.erase(socket->get_fd());
            break;
        case SHMT_WRITE:
            write_map.erase(socket->get_fd());
            break;
        case SHMT_EXCEPTION:
            exception_map.erase(socket->get_fd());
            break;
        default:
            throw std::runtime_error("unexpected map_type");
    }
}

// ソケットマップ sockmap をFD集合 socketset に変換する
void    EventLoop::prepare_fd_set(socket_map& sockmap, fd_set *sockset) {
    FD_ZERO(sockset);
    for (socket_map::iterator it = sockmap.begin(); it != sockmap.end(); it++) {
        FD_SET(it->first, sockset);
    }
}

// sockmap 中のソケットがFD集合 socketset に含まれるかどうかを調べ,
// 含まれている場合はソケットの run メソッドを実行する
void    EventLoop::scan_fd_set(socket_map& sockmap, fd_set *sockset) {
    for (EventLoop::socket_map::iterator it = sockmap.begin(); it != sockmap.end(); it++) {
        int fd = it->first;
        if (FD_ISSET(fd, sockset)) {
            it->second->run(*this);
        }
    }
}

// イベントループ
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

void    EventLoop::preserve(ISocket* socket, SocketHolderMapType from, SocketHolderMapType to) {
    SocketPreservation  pre = {
        .sock = socket,
        .from = from,
        .to = to,
    };
    up_queue.push_back(pre);
}

// 次のselectの前に, このソケットを監視対象から除外する
// (その際ソケットはdeleteされる)
void    EventLoop::preserve_clear(ISocket* socket, SocketHolderMapType from) {
    preserve(socket, from, SHMT_NONE);
}

// 次のselectの前に, このソケットを監視対象に追加する
void    EventLoop::preserve_set(ISocket* socket, SocketHolderMapType to) {
    preserve(socket, SHMT_NONE, to);
}

// 次のselectの前に, このソケットの監視方法を変更する
void    EventLoop::preserve_move(ISocket* socket, SocketHolderMapType from, SocketHolderMapType to) {
    preserve(socket, from, to);
}

// ソケットの監視状態変更予約を実施する
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
