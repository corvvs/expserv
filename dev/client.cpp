#include "connectedsocket.hpp"
#include <cstdlib>
#include <iostream>
#include <sstream>

void sendss(ConnectedSocket* sock, std::stringstream& ss) {
    usleep(rand() / 2000);
    std::string s = ss.str();
    sock->send(s.c_str(), s.length(), 0);
    ss.str("");
}

int main() {
    int fork_num = 2000;

    for (int i = 0; i < fork_num; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            pid = getpid();
            srand(pid);
            // std::cout << "sleep: " << sleeptime << std::endl;
            ConnectedSocket *sock = ConnectedSocket::connect(SD_IP4, ST_TCP, 8080 + pid % 5);
            // std::cout << "fd: " << sock->get_fd() << std::endl;
            std::stringstream ss;
            ss << "(" << i << ") ";
            sendss(sock, ss);
            ss << "this is hello from pid: ";
            sendss(sock, ss);
            ss << pid;
            sendss(sock, ss);
            ss << "\n";
            sendss(sock, ss);
            return 0;
        }
    }
    for (int i = 0; i < fork_num; i++) {
        wait(NULL);
    }
    return 0;
}
