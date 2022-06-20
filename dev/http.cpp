#include "http.hpp"

const HTTP::byte_string HTTP::version_str(HTTP::t_version version) {
    switch (version) {
        case V_0_9:
            return "HTTP/0.9";
        case V_1_0:
            return "HTTP/1.0";
        case V_1_1:
            return "HTTP/1.1";
        default:
            return "";
    }
}

const HTTP::byte_string HTTP::reason(HTTP::t_status status) {
    switch (status) {
        case HTTP::STATUS_OK:
            return "OK";
        case HTTP::STATUS_FOUND:
            return "Found";
        case HTTP::STATUS_BAD_REQUEST:
            return "Bad Request";
        case HTTP::STATUS_UNAUTHORIZED:
            return "Unauthorized";
        case HTTP::STATUS_FORBIDDEN:
            return "Forbidden";
        case HTTP::STATUS_NOT_FOUND:
            return "Not Found";
        case HTTP::STATUS_METHOD_NOT_ALLOWED:
            return "Method Not Allowed";
        case HTTP::STATUS_IM_A_TEAPOT:
            return "I'm a teapot";
        case HTTP::STATUS_INTERNAL_SERVER_ERROR:
            return "Internal Server Error";
        case HTTP::STATUS_NOT_IMPLEMENTED:
            return "Not Implemented";
        case HTTP::STATUS_BAD_GATEWAY:
            return "Bad Gateway";
        case HTTP::STATUS_SERVICE_UNAVAILABLE:
            return "Service Unavailable";
        case HTTP::STATUS_VERSION_NOT_SUPPORTED:
            return "HTTP Version Not Supported";
        default:
            return "";
    }
}

HTTP::CharFilter::CharFilter(const byte_string& chars) {
    fill(chars);
}

HTTP::CharFilter::CharFilter(const char *chars) {
    fill(byte_string(chars));
}

HTTP::CharFilter::CharFilter(const CharFilter& other) {
    *this = other;
}

HTTP::CharFilter& HTTP::CharFilter::operator=(const CharFilter& rhs) {
    if (this != &rhs) {
        memcpy(filter, rhs.filter, sizeof(uint64_t) * 4);
    }
    return *this;
}

HTTP::CharFilter& HTTP::CharFilter::operator=(const byte_string& rhs) {
    fill(rhs);
    return *this;
}

HTTP::CharFilter HTTP::CharFilter::operator|(const CharFilter& rhs) const {
    CharFilter n(*this);
    for (int i = 0; i < 4; ++i) {
        n.filter[i] |= rhs.filter[i];
    }
    return n;
}

HTTP::CharFilter HTTP::CharFilter::operator&(const CharFilter& rhs) const {
    CharFilter n(*this);
    for (int i = 0; i < 4; ++i) {
        n.filter[i] &= rhs.filter[i];
    }
    return n;
}

HTTP::CharFilter HTTP::CharFilter::operator^(const CharFilter& rhs) const {
    CharFilter n(*this);
    for (int i = 0; i < 4; ++i) {
        n.filter[i] ^= rhs.filter[i];
    }
    return n;
}

void HTTP::CharFilter::fill(const byte_string& chars) {
    memset(filter, 0, sizeof(uint64_t) * 4);
    // 6 の出どころは 2^6 = 64.
    for (byte_string::size_type i = 0; i < chars.size(); ++i) {
        uint8_t c = chars[i];
        filter[(c >> 6)] |= (uint64_t)1 << (c & (((uint64_t)1 << 6) - 1));
    }
}

bool HTTP::CharFilter::includes(uint8_t c) const {
    uint64_t x = (filter[(c >> 6)] & ((uint64_t)1 << (c & (((uint64_t)1 << 6) - 1))));
    return x;
}

HTTP::byte_string::size_type HTTP::CharFilter::size() const {
    byte_string::size_type n = 0;
    for (int i = 0; i < 4; ++i) {
        uint64_t x = filter[i];
        x = (x & 0x5555555555555555UL) + ((x & 0xaaaaaaaaaaaaaaaaUL) >> 1);
        x = (x & 0x3333333333333333UL) + ((x & 0xccccccccccccccccUL) >> 2);
        x = (x & 0x0f0f0f0f0f0f0f0fUL) + ((x & 0xf0f0f0f0f0f0f0f0UL) >> 4);
        x = (x & 0x00ff00ff00ff00ffUL) + ((x & 0xff00ff00ff00ff00UL) >> 8);
        x = (x & 0x0000ffff0000ffffUL) + ((x & 0xffff0000ffff0000UL) >> 16);
        x = (x & 0x00000000ffffffffUL) + ((x & 0xffffffff00000000UL) >> 32);
        n += x;
    }
    return n;
}

HTTP::byte_string   HTTP::CharFilter::str() const {
    HTTP::byte_string   out;
    for (unsigned int i = 0; i < 4 * 8 * sizeof(uint64_t); ++i) {
        uint8_t c = i;
        if (includes(c)) { out.push_back(c); }
    }
    return out;
}

std::ostream&   operator<<(std::ostream& ost, const HTTP::CharFilter& f) {
    return ost << "(" << f.size() << "):[" << f.str() << "]";
}


const HTTP::CharFilter HTTP::CharFilter::alpha_low  = HTTP::Charset::alpha_low;
const HTTP::CharFilter HTTP::CharFilter::alpha_up   = HTTP::Charset::alpha_up;
const HTTP::CharFilter HTTP::CharFilter::alpha      = HTTP::Charset::alpha;
const HTTP::CharFilter HTTP::CharFilter::digit      = HTTP::Charset::digit;
const HTTP::CharFilter HTTP::CharFilter::hexdig     = HTTP::Charset::hexdig;
const HTTP::CharFilter HTTP::CharFilter::unreserved = HTTP::Charset::unreserved;
const HTTP::CharFilter HTTP::CharFilter::gen_delims = HTTP::Charset::gen_delims;
const HTTP::CharFilter HTTP::CharFilter::sub_delims = HTTP::Charset::sub_delims;
