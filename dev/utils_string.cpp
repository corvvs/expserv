#include "utils_string.hpp"

HTTP::byte_string HTTP::Utils::downcase(const byte_string &str) {
    HTTP::byte_string rv(str.size(), 0);
    for (byte_string::size_type i = 0; i < str.size(); ++i) {
        rv[i] = tolower(str[i]);
    }
    return rv;
}

HTTP::byte_string &HTTP::Utils::downcase(byte_string &str) {
    for (byte_string::size_type i = 0; i < str.size(); ++i) {
        str[i] = tolower(str[i]);
    }
    return str;
}

HTTP::byte_string HTTP::Utils::upcase(const byte_string &str) {
    HTTP::byte_string rv(str.size(), 0);
    for (byte_string::size_type i = 0; i < str.size(); ++i) {
        rv[i] = toupper(str[i]);
    }
    return rv;
}

void HTTP::Utils::upcase(byte_string &str) {
    for (byte_string::size_type i = 0; i < str.size(); ++i) {
        str[i] = toupper(str[i]);
    }
}
