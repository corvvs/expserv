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
