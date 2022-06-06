#ifndef ERRORHTTP_HPP
# define ERRORHTTP_HPP
# include "test_common.hpp"
# include "http.hpp"
# include <exception>
# include <stdexcept>

class http_error: public std::runtime_error {
    private:
        HTTP::t_status   status_;
    public:
        http_error(const char *_Message, HTTP::t_status status);

        HTTP::t_status   get_status() const;
};

#endif
