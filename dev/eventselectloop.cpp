#include "eventselectloop.hpp"

void EventSelectLoop::destroy_all(EventSelectLoop::socket_map &m) {
    for (EventSelectLoop::socket_map::iterator it = m.begin(); it != m.end(); it++) {
        delete it->second;
    }
}

EventSelectLoop::EventSelectLoop() {}

EventSelectLoop::~EventSelectLoop() {
    destroy_all(read_map);
    destroy_all(write_map);
    destroy_all(exception_map);
}

void EventSelectLoop::watch(ISocketLike *socket, t_socket_operation map_type) {
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

void EventSelectLoop::unwatch(ISocketLike *socket, t_socket_operation map_type) {
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
void EventSelectLoop::prepare_fd_set(socket_map &sockmap, fd_set *sockset) {
    FD_ZERO(sockset);
    for (socket_map::iterator it = sockmap.begin(); it != sockmap.end(); it++) {
        FD_SET(it->first, sockset);
    }
}

// sockmap 中のソケットがFD集合 socketset に含まれるかどうかを調べ,
// 含まれている場合はソケットの notify メソッドを実行する
void EventSelectLoop::scan_fd_set(socket_map &sockmap, fd_set *sockset, t_time_epoch_ms now) {
    for (EventSelectLoop::socket_map::iterator it = sockmap.begin(); it != sockmap.end(); it++) {
        if (now == 0) {
            it->second->timeout(*this, now);
        } else {
            if (FD_ISSET(it->first, sockset)) {
                it->second->notify(*this);
            }
        }
    }
}

// イベントループ
void EventSelectLoop::loop() {
    fd_set read_set;
    fd_set write_set;
    fd_set exception_set;
    struct timeval tv;

    while (1) {
        update();
        prepare_fd_set(read_map, &read_set);
        prepare_fd_set(write_map, &write_set);
        prepare_fd_set(exception_map, &exception_set);

        tv.tv_sec  = 10;
        tv.tv_usec = 0;

        t_fd max_fd = -1;
        if (!read_map.empty()) {
            max_fd = std::max(max_fd, read_map.rbegin()->first);
        }
        if (!write_map.empty()) {
            max_fd = std::max(max_fd, write_map.rbegin()->first);
        }
        if (!exception_map.empty()) {
            max_fd = std::max(max_fd, exception_map.rbegin()->first);
        }

        int count = select(max_fd + 1, &read_set, &write_set, &exception_set, &tv);
        if (count < 0) {
            DXOUT(strerror(errno));
            throw std::runtime_error("select error");
        } else if (count == 0) {
            t_time_epoch_ms now = WSTime::get_epoch_ms();
            DXOUT("timeout?: " << now);
            scan_fd_set(read_map, &read_set, now);
            scan_fd_set(write_map, &write_set, now);
            scan_fd_set(exception_map, &exception_set, now);
        } else {
            scan_fd_set(read_map, &read_set, 0);
            scan_fd_set(write_map, &write_set, 0);
            scan_fd_set(exception_map, &exception_set, 0);
        }
    }
}

void EventSelectLoop::reserve(ISocketLike *socket, t_socket_operation from, t_socket_operation to) {
    t_socket_reservation pre = {socket, from, to};
    up_queue.push_back(pre);
}

// 次のselectの前に, このソケットを監視対象から除外する
// (その際ソケットはdeleteされる)
void EventSelectLoop::reserve_clear(ISocketLike *socket, t_socket_operation from) {
    reserve(socket, from, SHMT_NONE);
}

// 次のselectの前に, このソケットを監視対象に追加する
void EventSelectLoop::reserve_set(ISocketLike *socket, t_socket_operation to) {
    reserve(socket, SHMT_NONE, to);
}

// 次のselectの前に, このソケットの監視方法を変更する
void EventSelectLoop::reserve_transit(ISocketLike *socket, t_socket_operation from, t_socket_operation to) {
    reserve(socket, from, to);
}

// ソケットの監視状態変更予約を実施する
void EventSelectLoop::update() {
    for (EventSelectLoop::update_queue::iterator it = up_queue.begin(); it != up_queue.end(); it++) {
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
