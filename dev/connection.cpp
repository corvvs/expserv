#include "connection.hpp"
#define N 1024

Connection::Connection() {
    throw std::runtime_error("forbidden");
}

Connection::Connection(SocketListening* listening)
    : phase(CONNECTION_NEUTRAL), dying(false), current_req(NULL) {
    sock = listening->accept();
}

Connection::~Connection() {
    delete sock;
}

t_fd    Connection::get_fd() const {
    return sock->get_fd();
}

void    Connection::notify(IPanopticon& loop) {
    if (dying) { return; }

    int fd = sock->get_fd();
    std::cout << "[S]" << fd << " rc: " << phase << std::endl;

    switch (phase) {
        case CONNECTION_NEUTRAL:
        case CONNECTION_RECEIVING: {
            try {
                char buf[RequestHTTP::MAX_REQLINE_END];
                ssize_t receipt = sock->receive(&buf, 3, 0);
                if (receipt <= 0) {
                    std::cout << "[S]" << fd << " * closed *" << std::endl;
                    loop.preserve_clear(this, SHMT_READ);
                    phase = CONNECTION_RESPONDING;
                    return;
                }

                std::cout << "[S]" << fd << " receipt: " << receipt << std::endl;
                if (current_req == NULL) {
                    current_req = new RequestHTTP();
                }
                current_req->feed_bytes(buf, receipt);
                if (current_req->respond_ready()) {

                    phase = CONNECTION_RESPONDING;
                    loop.preserve_move(this, SHMT_READ, SHMT_WRITE);
                }
                return;
            } catch (http_error err) {
                // HTTPエラーを受け取ったら,
                // - エラー応答モードに入る
                // - エラーレスポンスを生成
                // - ソケットを送信モードで監視
                std::cout << "[SE] catched an http error: " << err.get_status() << ": " << err.what() << std::endl;
                phase = CONNECTION_ERROR_RESPONDING;
                loop.preserve_move(this, SHMT_READ, SHMT_WRITE);
                return;
            }
        }
        // case CONNECTION_RESPONDING: {

        //     return;
        // }
        default:
            std::cout << "FAIL: " << fd << std::endl;
            throw std::runtime_error("????");
    }

    // switch (run_counter) {
    //     case 0: {
    //         char buf[N];
    //         ssize_t receipt = sock->receive(&buf, N, 0);
    //         if (receipt <= 0) {
    //             std::cout << "[S]" << receipt_str;
    //             std::cout << " fd: " << fd;
    //             std::cout << " port: " << sock->get_port();
    //             std::cout << std::endl;
    //             receipt_str.clear();

    //             // loop.preserve_clear(this, SHMT_WRITE);
    //             loop.preserve_move(this, SHMT_READ, SHMT_WRITE);
    //             run_counter++;
    //             return;
    //         }
    //         receipt_str += std::string(buf, receipt);
    //         return;
    //     }
    //     case 1: {
    //         // レスポンス返す処理
    //         const std::string rs = "hello back";
    //         sock->send(rs.c_str(), rs.length(), 0);
    //         loop.preserve_clear(this, SHMT_WRITE);
    //         dying = true;
    //         run_counter++;
    //         return;
    //     }
    //     default:
    //         std::cout << "FAIL: " << fd << std::endl;
    //         throw std::runtime_error("????");
    // }
}
