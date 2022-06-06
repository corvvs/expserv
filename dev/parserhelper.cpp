#include "parserhelper.hpp"

ParserHelper::index_range ParserHelper::find_crlf(const byte_string& str, ssize_t from, ssize_t len) {
    for (ssize_t i = from; i - from < len; i++) {
        // DSOUT() << from << ", " << i << ", " << len << ": " << str[i] << "-" << int(str[i]) << std::endl;
        if (str[i] == '\n') {
            if (0 < i && str[i - 1] == '\r') {
                return index_range(i - 1 - from, i + 1 - from);
            }
            return index_range(i - from, i + 1 - from);
        }
    }
    return index_range(len, len);
}

ParserHelper::index_range ParserHelper::find_blank_line(const byte_string& str, ssize_t from, ssize_t len) {
    ssize_t i;
    for(i = 0; i < len;) {
        // DOUT() << "i: " << i << std::endl;
        index_range nl = find_crlf(str, from + i, len - i);
        // DOUT() << "nl: [" << nl.first << "," << nl.second << ")" << std::endl;
        if (nl.first >= nl.second) {
            break;
        }
        i += nl.second;
        if (i + 1 < len && str[i + 1 + from] == '\r') {
            i += 1;
        }
        if (i + 1 < len && str[i + 1 + from] == '\n') {
            return index_range(i - (nl.second - nl.first), i + 1);
        }
    }
    return index_range(len, len);
}

ssize_t      ParserHelper::ignore_crlf(const byte_string& str, ssize_t from, ssize_t len) {
    ssize_t i = 0;
    for (; i < len; i++) {
        // DOUT() << i << ": " << str[i + from] << std::endl;
        if (str[i + from] == '\n') {
            continue;
        } else if (str[i + from] == '\r') {
            if (i + 1 < len && str[i + from] == '\n') {
                continue;
            }
        }
        break;
    }
    // DOUT() << "result: " << i << std::endl;
    return i;
}

bool        ParserHelper::is_sp(char c) {
    return c == ' ';
}

ssize_t      ParserHelper::ignore_sp(const byte_string& str, ssize_t from, ssize_t len) {
    ssize_t i = 0;
    for (; i < len; i++) {
        if (is_sp(str[i + from])) {
            continue;
        }
        break;
    }
    return i;
}

ssize_t      ParserHelper::ignore_not_sp(const byte_string& str, ssize_t from, ssize_t len) {
    ssize_t i = 0;
    for (; i < len; i++) {
        if (!is_sp(str[i + from])) {
            continue;
        }
        break;
    }
    return i;
}

std::vector< ParserHelper::byte_string >  ParserHelper::split_by_sp(
    ParserHelper::byte_string::const_iterator first,
    ParserHelper::byte_string::const_iterator last
) {
    typedef std::basic_string<char> str_type;
    std::vector< str_type >  rv;
    str_type::size_type len = std::distance(first, last);
    str_type::size_type i = 0;
    str_type::size_type word_from = 0;
    bool prev_is_sp = true;
    for (; i <= len; i++) {
        bool cur_is_sp = i == len || is_sp(*(first + i));
        if (cur_is_sp && !prev_is_sp) {
            rv.push_back(str_type(
                first + word_from, first + i
            ));
        } else if (!cur_is_sp && prev_is_sp) {
            word_from = i;
        }
        prev_is_sp = cur_is_sp;
    }
    return rv;
}

bool    ParserHelper::is_listing_header(const byte_string& key) {
    if (key == "set-cookie") { return true; }
    return false;
}

unsigned int    ParserHelper::stou(const byte_string& str) {
    std::stringstream   ss;
    ss << str;
    unsigned int        v;
    ss >> v;
    ss.clear();
    ss << v;
    byte_string         r;
    ss >> r;
    if (str != r) {
        throw std::runtime_error("failed to convert string to int");
    }
    return v;
}

ParserHelper::byte_string   ParserHelper::utos(unsigned int u) {
    std::stringstream   ss;
    ss << u;
    return ss.str();
}
