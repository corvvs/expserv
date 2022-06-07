#include "connection.hpp"
#include "channel.hpp"

Connection::Connection() {
    throw std::runtime_error("forbidden");
}

Connection::Connection(
    IRouter* router,
    SocketConnected* sock_given
):
    router_(router),
    phase(CONNECTION_RECEIVING),
    dying(false),
    sock(sock_given),
    current_req(NULL) {}

Connection::~Connection() {
    delete sock;
    delete current_req;
    delete current_res;
}

t_fd    Connection::get_fd() const {
    return sock->get_fd();
}

void    Connection::notify(IObserver& loop) {
    // switch, try-catch は意図的にインデント崩して書いてます
    if (dying) { return; }

    int fd = sock->get_fd();
    std::cout << "[S]" << fd << " rc: " << phase << std::endl;

    const size_t  read_buffer_size = RequestHTTP::MAX_REQLINE_END;
    char buf[read_buffer_size];

switch (phase) {
case CONNECTION_RECEIVING: {
    // [コネクション:受信モード]

    try {
        ssize_t receipt = sock->receive(&buf, read_buffer_size, 0);
        if (receipt <= 0) {
            std::cout << "[S]" << fd << " * closed *" << std::endl;
            loop.reserve_clear(this, SHMT_READ);
            phase = CONNECTION_RESPONDING;
            return;
        }

        if (current_req == NULL) {
            // リクエストオブジェクトを作成して解析開始
            current_req = new RequestHTTP();
        }
        current_req->feed_bytestring(buf, receipt);
        if (!current_req->respond_ready()) { return; }

        // リクエストの解析が完了したら応答開始
        current_res = router_->route(current_req);
        loop.reserve_transit(this, SHMT_READ, SHMT_WRITE);
        phase = CONNECTION_RESPONDING;

    } catch (http_error err) {

        // 受信中のHTTPエラー
        std::cout << "[SE] catched an http error: " << err.get_status() << ": " << err.what() << std::endl;
        phase = CONNECTION_ERROR_RESPONDING;
        current_res = router_->respond_error(current_req, err);
        loop.reserve_transit(this, SHMT_READ, SHMT_WRITE);

    }
    return;

}

case CONNECTION_ERROR_RESPONDING:
case CONNECTION_RESPONDING: {
    // [コネクション:送信モード]
    try {

        const char *buf = current_res->get_unsent();
        ssize_t sent = sock->send(buf, current_res->get_unsent_size(), 0);
        DSOUT() << "sending " << current_res->get_unsent_size() << "bytes, and actually sent " << sent << "bytes" << std::endl;

        if (sent > 0) {
            current_res->mark_sent(sent);
        }
        // ソケットを閉じる
        if (sent <= 0 || current_res->get_unsent_size() == 0) {
            loop.reserve_clear(this, SHMT_WRITE);
            phase = CONNECTION_RECEIVING;
        }
        // TODO: keep-alive対応

    } catch (http_error err) {

        // 送信中のHTTPエラー
        DSOUT() << "[SE] catched an http error: " << err.get_status() << ": " << err.what() << std::endl;
        clear_currents();
        loop.reserve_transit(this, SHMT_WRITE, SHMT_READ);
        phase = CONNECTION_RECEIVING;

    }
    return;
}

default: {
    DSOUT() << "unexpected phase: " << phase << std::endl;
    throw std::runtime_error("????");
}
}
}

void    Connection::clear_currents() {
    delete current_res;
    current_res = NULL;
    delete current_req;
    current_req = NULL;
}
