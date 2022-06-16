#include "parserhelper.hpp"

ParserHelper::index_range::index_range(ssize_t f, ssize_t t):
    pair(f, t) {}

ParserHelper::index_range  ParserHelper::index_range::invalid() {
    return index_range(0, -1);
}

bool    ParserHelper::index_range::is_invalid() const {
    return first > second;
}

ssize_t ParserHelper::index_range::length() const {
    return second - first;
}

ParserHelper::index_range ParserHelper::find_crlf(const byte_string& str, ssize_t from, ssize_t len) {
    for (ssize_t i = from; i - from < len; i++) {
        // iは絶対インデックス; strの先頭からの位置
        ssize_t ri = i - from;
        // riは相対インデックス; fromからの位置
        // DSOUT() << from << ", " << i << ", " << len << ": " << str[i] << "-" << int(str[i]) << std::endl;
        if (str[i] == '\n') {
            if (0 < i && str[i - 1] == '\r') {
                return index_range(ri - 1, ri + 1);
            }
            return index_range(ri, ri + 1);
        }
    }
    return index_range(len+1,len);
}

ParserHelper::index_range ParserHelper::find_crlf_header_value(const byte_string& str, ssize_t from, ssize_t len) {
    DSOUT() << "target: \"" << byte_string(str.begin() + from, str.begin() + from + len) << "\"" << std::endl;
    ssize_t movement = 0;
    while (true) {
        ssize_t rfrom = from + movement;
        ssize_t rlen = len - movement;
        DSOUT() << "finding from: " << rfrom << ", len: " << rlen << std::endl;
        index_range ir = find_crlf(str, rfrom, rlen);
        if (!ir.is_invalid()) {
            // is obs-fold ?
            DSOUT() << "is obs-fold?? " << ir << std::endl;
            if (ir.second < len && is_sp(str[from + ir.second])) {
                DSOUT() << "is obs-fold!!" << std::endl;
                movement += ir.second;
                continue;
            }
            return index_range(ir.first + movement, ir.second + movement);
        }
        break;
    }
    return index_range(len + 1 + movement,len + movement);
}

ParserHelper::index_range ParserHelper::find_blank_line(const byte_string& str, ssize_t from, ssize_t len) {
    ssize_t i;
    for(i = 0; i < len;) {
        // DOUT() << "i: " << i << std::endl;
        index_range nl = find_crlf(str, from + i, len - i);
        // DOUT() << "nl: [" << nl.first << "," << nl.second << ")" << std::endl;
        if (nl.is_invalid()) {
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
    return index_range(len+1,len);
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
    return SP.find(c) != std::string::npos;
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

void    ParserHelper::normalize_header_key(byte_string& key) {
    for (byte_string::iterator it = key.begin(); it != key.end(); it++) {
        *it = tolower(*it);
    }
}


bool    ParserHelper::is_listing_header(const byte_string& key) {
    if (key == "set-cookie") { return true; }
    return false;
}

unsigned int    ParserHelper::stou(const byte_string& str) {
    std::stringstream   ss;
    byte_string         r;
    unsigned int        v;

    ss << str; ss >> v;
    ss.clear();
    ss << v; ss >> r;
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

std::ostream&   operator<<(std::ostream& out, const ParserHelper::index_range& r) {
    return out << "[" << r.first << "," << r.second << ")";
}
