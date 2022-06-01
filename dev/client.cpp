#include "connectedsocket.hpp"
#include <cstdlib>
#include <iostream>
#include <sstream>

void sendss(int i, ConnectedSocket* sock, std::stringstream& ss) {
    usleep(rand() / 2000);
    std::string s = ss.str();
    sock->send(s.c_str(), s.length(), 0);
    std::cout << "C[" << i << "] client sent: " << s << std::endl;
    ss.str("");
}

int main() {
    int fork_num = 4000;

    for (int i = 0; i < fork_num; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            pid = getpid();
            srand(pid);
            // std::cout << "sleep: " << sleeptime << std::endl;
            ConnectedSocket *sock = ConnectedSocket::connect(SD_IP4, ST_TCP, 8080 + pid % 5);
            // std::cout << "fd: " << sock->get_fd() << std::endl;
            std::stringstream ss;
            ss << i << ": ";
            sendss(i, sock, ss);
            ss << "this is hello from pid: ";
            sendss(i, sock, ss);
            ss << pid;
            sendss(i, sock, ss);
            std::cout << "C[" << i << "] sending over" << std::endl;
            shutdown(sock->get_fd(), SHUT_WR);
            std::cout << "C[" << i << "] shutted down" << std::endl;
            char buf[4];
            std::string rs = "";
            while (true) {
                ssize_t n = sock->receive(buf, 4, 0);
                if (n <= 0) {
                    break;
                }
                rs += std::string(buf, n);
            }
            std::cout << "C" << i << ": receipt from server: " << rs << std::endl;
            return 0;
        }
    }
    for (int i = 0; i < fork_num; i++) {
        wait(NULL);
    }
    return 0;
}
