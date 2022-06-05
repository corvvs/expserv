# include "http_error.hpp"

http_error::http_error(const char *_Message, t_http_status status)
    :runtime_error(_Message), status_(status) {}

t_http_status   http_error::get_status() const {
    return status_;
}
