#include "connection.hpp"
#include "channel.hpp"

Connection::Connection() {
    throw std::runtime_error("forbidden");
}

Connection::Connection(SocketConnected* sock_given):
    phase(CONNECTION_RECEIVING), dying(false),
    sock(sock_given),
    current_req(NULL) {
}

Connection::~Connection() {
    delete sock;
}

t_fd    Connection::get_fd() const {
    return sock->get_fd();
}

void    Connection::notify(IObserver& loop) {
    if (dying) { return; }

    int fd = sock->get_fd();
    std::cout << "[S]" << fd << " rc: " << phase << std::endl;

    const size_t  read_buffer_size = RequestHTTP::MAX_REQLINE_END;
    // const size_t  read_buffer_size = 3;
    char buf[read_buffer_size];
    switch (phase) {
        case CONNECTION_RECEIVING: {

            try {
                ssize_t receipt = sock->receive(&buf, read_buffer_size, 0);
                if (receipt <= 0) {
                    std::cout << "[S]" << fd << " * closed *" << std::endl;
                    loop.preserve_clear(this, SHMT_READ);
                    phase = CONNECTION_RESPONDING;
                    return;
                }

                std::cout << "[S]" << fd << " receipt: " << receipt << std::endl;
                if (current_req == NULL) {
                    // リクエストオブジェクトを作成して解析開始
                    current_req = new RequestHTTP();
                }
                current_req->feed_bytes(buf, receipt);
                if (current_req->respond_ready()) {
                    // リクエストの解析が完了したら応答開始
                    phase = CONNECTION_RESPONDING;
                    
                    current_res = new ResponseHTTP(
                        HTTP::DEFAULT_HTTP_VERSION,
                        HTTP::STATUS_OK
                    );

                    current_res->feed_body(
                        current_req->get_body_begin(),
                        current_req->get_body_end()
                    );
                    current_res->render();
                    DSOUT() << current_res->get_message_text() << std::endl;
                    loop.preserve_move(this, SHMT_READ, SHMT_WRITE);
                }
            } catch (http_error err) {
                // HTTPエラーを受け取ったら,
                // - エラー応答モードに入る
                // - エラーレスポンスを生成
                // - ソケットを送信モードで監視
                std::cout << "[SE] catched an http error: " << err.get_status() << ": " << err.what() << std::endl;
                phase = CONNECTION_ERROR_RESPONDING;

                current_res = new ResponseHTTP(
                    HTTP::DEFAULT_HTTP_VERSION,
                    err
                );
                current_res->render();
                loop.preserve_move(this, SHMT_READ, SHMT_WRITE);
            }
            return;

        }

        case CONNECTION_ERROR_RESPONDING:
        case CONNECTION_RESPONDING: {
            try {
                const char *buf = current_res->get_unsent();
                DSOUT() << "sending " << current_res->get_unsent_size() << "bytes" << std::endl;
                ssize_t sent = sock->send(buf, current_res->get_unsent_size(), 0);
                DSOUT() << "sent " << sent << "bytes" << std::endl;

                if (sent > 0) {
                    current_res->mark_sent(sent);
                }
                if (sent <= 0 || current_res->get_unsent_size() == 0) {
                    clear_currents();
                    loop.preserve_clear(this, SHMT_WRITE);
                }
            } catch (http_error err) {
                // 応答送信中に例外が起きたら, 応答を放棄する
                DSOUT() << "[SE] catched an http error: " << err.get_status() << ": " << err.what() << std::endl;
                clear_currents();
                loop.preserve_move(this, SHMT_WRITE, SHMT_READ);
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
    phase = CONNECTION_RECEIVING;
}
