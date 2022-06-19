#include "parserhelper.hpp"

IndexRange ParserHelper::find_crlf(const byte_string& str, ssize_t from, ssize_t len) {
    for (ssize_t i = from; i - from < len; i++) {
        // iは絶対インデックス; strの先頭からの位置
        // DSOUT() << from << ", " << i << ", " << len << ": " << str[i] << "-" << int(str[i]) << std::endl;
        if (str[i] == '\n') {
            if (0 < i && str[i - 1] == '\r') {
                return IndexRange(i - 1, i + 1);
            }
            return IndexRange(i, i + 1);
        }
    }
    return IndexRange(from + len + 1, from + len);
}

IndexRange ParserHelper::find_crlf_header_value(const byte_string& str, ssize_t from, ssize_t len) {
    // DSOUT() << "target: \"" << byte_string(str.begin() + from, str.begin() + from + len) << "\"" << std::endl;
    ssize_t movement = 0;
    while (true) {
        ssize_t rfrom = from + movement;
        ssize_t rlen = len - movement;
        // DSOUT() << "finding from: " << rfrom << ", len: " << rlen << std::endl;
        IndexRange r = find_crlf(str, rfrom, rlen);
        if (!r.is_invalid()) {
            // is obs-fold ?
            // DSOUT() << "is obs-fold?? " << r << std::endl;
            if (r.second < from + len && is_sp(str[r.second])) {
                DSOUT() << "is obs-fold!!" << std::endl;
                movement = r.second - from;
                continue;
            }
            return r;
        }
        break;
    }
    return IndexRange(from + len + 1, from + len);
}

IndexRange ParserHelper::find_obs_fold(const byte_string& str, ssize_t from, ssize_t len) {
    ssize_t movement = 0;
    while (true) {
        ssize_t rfrom = from + movement;
        ssize_t rlen = len - movement;
        IndexRange r = find_crlf(str, rfrom, rlen);
        if (!r.is_invalid()) {
            ssize_t i;
            for (i = 0; r.second + i < from + len && is_sp(str[r.second + i]); ++i);
            DSOUT() << r << " i: " << i << std::endl;
            if (i < 1) {
                // from + movement = r.second
                movement = r.second - from;
                continue;
            }
            // obs-fold!!
            return IndexRange(r.first, r.second + i);
        }
        return r;
    }
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
