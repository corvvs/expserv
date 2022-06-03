#ifndef PARSERHELPER_HPP
# define PARSERHELPER_HPP
# include <string>
# include <vector>
# include <utility>
# include "test_common.hpp"

namespace ParserHelper {
    typedef std::basic_string<char>         byte_buffer;
    typedef std::pair<ssize_t, ssize_t>     index_range;
    index_range find_crlf(char *bytes, ssize_t len);
    index_range find_blank_line(char *bytes, ssize_t len);
    ssize_t      ignore_crlf(char *bytes, ssize_t len);
    bool        is_sp(char c);
    ssize_t      ignore_sp(char *bytes, ssize_t len);
    ssize_t      ignore_not_sp(char *bytes, ssize_t len);
    std::vector< byte_buffer >  split_by_sp(
        byte_buffer::iterator first,
        byte_buffer::iterator last
    );
}

#endif
