#ifndef ERRORHTTP_HPP
# define ERRORHTTP_HPP
# include "test_common.hpp"
# include "http.hpp"
# include <exception>
# include <stdexcept>

class http_error: public std::runtime_error {
    private:
        t_http_status   status_;
    public:
        http_error(const char *_Message, t_http_status status);

        t_http_status   get_status() const;
};

#endif
