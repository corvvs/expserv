#include "cgi.hpp"
#include <unistd.h>

int redirect_fd(t_fd from, t_fd to) {
    if (from == to) {
        return 0;
    }
    close(to);
    return dup2(from, to);
}

CGI::CGI(pid_t pid,
         SocketUNIX *sock,
         byte_string &script_path,
         byte_string &query_string,
         metavar_dict_type &metavar,
         size_type content_length)
    : cgi_pid(pid)
    , sock(sock)
    , script_path_(script_path)
    , query_string_(query_string)
    , metavar_(metavar)
    , content_length_(content_length) {}

CGI::~CGI() {
    delete sock;
    // TODO: ちゃんとトラップする
    wait(NULL);
}

CGI *CGI::start_cgi(byte_string &script_path,
                    byte_string &query_string,
                    metavar_dict_type &metavar,
                    size_type content_length) {

    // TODO: CGIが起動できるかどうかチェックする

    std::pair<SocketUNIX *, t_fd> socks = SocketUNIX::socket_pair();

    pid_t pid = fork();
    if (pid < 0) {
        throw std::runtime_error("failed to fork");
    }
    if (pid == 0) {
        // child: CGI process
        delete socks.first;
        char **argv = flatten_argv(script_path);
        if (argv == NULL) {
            exit(1);
        }
        char **mvs = flatten_metavar(metavar);
        if (mvs == NULL) {
            exit(1);
        }
        if (redirect_fd(socks.second, STDIN_FILENO) < 0) {
            exit(1);
        }
        if (redirect_fd(socks.second, STDOUT_FILENO) < 0) {
            exit(1);
        }
        if (redirect_fd(socks.second, STDERR_FILENO) < 0) {
            exit(1);
        }
        // TODO: execve
        execve(HTTP::restrfy(script_path).c_str(), argv, mvs);
        exit(0);
    }
    // parent: server process
    close(socks.second);
    CGI *cgi = new CGI(pid, socks.first, script_path, query_string, metavar, content_length);
    return cgi;
}

char **CGI::flatten_argv(const byte_string &script_path) {
    size_t n     = 2;
    char **frame = (char **)malloc(sizeof(char *) * n);
    if (frame == NULL) {
        return frame;
    }
    frame[0] = strdup(HTTP::restrfy(script_path).c_str());
    if (frame[0] == NULL) {
        free(frame);
        return NULL;
    }
    frame[1] = NULL;
    return frame;
}

char **CGI::flatten_metavar(const metavar_dict_type &metavar) {
    size_t n     = metavar.size();
    char **frame = (char **)malloc(sizeof(char *) * (n + 1));
    if (frame == NULL) {
        return frame;
    }
    size_t i = 0;
    for (metavar_dict_type::const_iterator it = metavar.begin(); it != metavar.end(); ++it, ++i) {
        size_t j   = it->first.size() + 1 + it->second.size();
        char *item = (char *)malloc(sizeof(char) * (j + 1));
        if (item == NULL) {
            for (size_t k = 0; k < i; ++k) {
                free(frame[k]);
            }
            free(frame);
            return NULL;
        }
        memcpy(item, &(it->first.front()), it->first.size());
        item[it->first.size()] = '=';
        memcpy(item + it->first.size() + 1, &(it->second.front()), it->second.size());
        frame[i] = item;
    }
    return frame;
}

t_fd CGI::get_fd() const {
    return sock ? sock->get_fd() : -1;
}

void CGI::notify(IObserver &observer) {
    (void)observer;
}

void CGI::timeout(IObserver &observer, t_time_epoch_ms epoch) {
    (void)observer;
    (void)epoch;
}
