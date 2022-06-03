#include "connection.hpp"
#define N 1024

Connection::Connection() {
    throw std::runtime_error("forbidden");
}

Connection::Connection(SocketListening* listening)
    : run_counter(0), dying(false), current_req(NULL) {
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
    std::cout << "[S]" << fd << " rc: " << run_counter << std::endl;

    switch (run_counter) {
        case 0: {
            char buf[N];
            ssize_t receipt = sock->receive(&buf, N, 0);
            std::cout << "[S]" << fd << " receipt: " << receipt << std::endl;
            if (receipt <= 0) {
                loop.preserve_clear(this, SHMT_READ);
                run_counter++;
                return;
            }

            if (current_req == NULL) {
                current_req = new RequestHTTP();
            }
            current_req->feed_bytes(buf, receipt);
            return;
        }
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
