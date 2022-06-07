#include "connection.hpp"
#include "channel.hpp"

int started = 0;
int finished = 0;

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
    current_req(NULL),
    current_res(NULL)
{
    started_ = started++;
    DSOUT() << "started_: " << started_ << std::endl;
}

Connection::~Connection() {
    delete sock;
    delete current_req;
    delete current_res;
    DSOUT() << "finished: " << finished++ << " - " << started_ << std::endl;
}

t_fd    Connection::get_fd() const {
    return sock->get_fd();
}

void    Connection::notify(IObserver& observer) {
    // switch, try-catch は意図的にインデント崩して書いてます
    if (dying) { return; }

    int fd = sock->get_fd();
    // std::cout << "[S]" << fd << " rc: " << phase << std::endl;

    const size_t  read_buffer_size = RequestHTTP::MAX_REQLINE_END;
    char buf[read_buffer_size];

switch (phase) {
case CONNECTION_RECEIVING: {
    // [コネクション:受信モード]

    try {
        ssize_t receipt = sock->receive(&buf, read_buffer_size, 0);
        if (receipt <= 0) {
            // なにも受信できなかったか, 受信エラーが起きた場合
            std::cout << "[S]" << fd << " * closing... *" << std::endl;
            die(observer);
            return;
        }

        if (current_req == NULL) {
            // リクエストオブジェクトを作成して解析開始
            current_req = new RequestHTTP();
        }
        current_req->feed_bytestring(buf, receipt);
        if (!current_req->is_ready_to_respond()) { return; }

        // リクエストの解析が完了したら応答開始
        current_res = router_->route(current_req);
        observer.reserve_transit(this, SHMT_READ, SHMT_WRITE);
        phase = CONNECTION_RESPONDING;

    } catch (http_error err) {

        // 受信中のHTTPエラー
        std::cout << "[SE] catched an http error: " << err.get_status() << ": " << err.what() << std::endl;
        current_res = router_->respond_error(current_req, err);
        observer.reserve_transit(this, SHMT_READ, SHMT_WRITE);
        phase = CONNECTION_ERROR_RESPONDING;

    }
    return;

}

case CONNECTION_ERROR_RESPONDING:
case CONNECTION_RESPONDING: {
    // [コネクション:送信モード]
    try {

        const char *buf = current_res->get_unsent_head();
        ssize_t sent = sock->send(buf, current_res->get_unsent_size(), 0);
        // DSOUT() << "sending " << current_res->get_unsent_size() << "bytes, and actually sent " << sent << "bytes" << std::endl;

        // 送信ができなかったか, エラーが起きた場合
        if (sent <= 0) {
            // TODO: とりあえず接続を閉じておくが, 本当はどうするべき？
            die(observer);
            return;
        }

        current_res->mark_sent(sent);
        if (!current_res->is_over_sending()) { return; }

        // 送信完了
        if (current_req->should_keep_in_touch()) {
            // 接続を維持する
            restart(observer);
            return;
        } else {
            die(observer);
            return;
        }

    } catch (http_error err) {

        // 送信中のHTTPエラー
        DSOUT() << "[SE] catched an http error: " << err.get_status() << ": " << err.what() << std::endl;
        restart(observer);

    }
    return;
}

default: {
    DSOUT() << "unexpected phase: " << phase << std::endl;
    throw std::runtime_error("????");
}
}
}

void    Connection::restart(IObserver& observer) {
    delete current_res;
    delete current_req;
    current_res = NULL;
    current_req = NULL;
    observer.reserve_transit(this, SHMT_WRITE, SHMT_READ);
    phase = CONNECTION_RECEIVING;
}

void    Connection::die(IObserver& observer) {
    observer.reserve_clear(this, SHMT_WRITE);
    dying = true;
}
