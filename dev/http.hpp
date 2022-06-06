#ifndef HTTP_HPP
# define HTTP_HPP
# include <string>
# include <map>

namespace HTTP {
    enum t_status {
        STATUS_OK = 200,

        STATUS_FOUND = 302,

        STATUS_BAD_REQUEST = 400,
        STATUS_UNAUTHORIZED = 401,
        STATUS_FORBIDDEN = 403,
        STATUS_NOT_FOUND = 404,
        STATUS_METHOD_NOT_ALLOWED = 405,
        STATUS_URI_TOO_LONG = 414,
        STATUS_IM_A_TEAPOT = 418,

        STATUS_INTERNAL_SERVER_ERROR = 500,
        STATUS_NOT_IMPLEMENTED = 501,
        STATUS_BAD_GATEWAY = 502,
        STATUS_SERVICE_UNAVAILABLE = 503,
        STATUS_VERSION_NOT_SUPPORTED = 505,
        STATUS_DUMMY = 0
    };

    enum t_method {
        METHOD_UNKNOWN,
        METHOD_GET,
        METHOD_POST,
        METHOD_DELETE,
        METHOD_ERROR
    };

    enum t_version {
        V_UNKNOWN,
        V_0_9,
        V_1_0,
        V_1_1,
        V_ERROR
    };

    typedef std::basic_string<char> byte_string;
    typedef byte_string             header_key_type;
    typedef byte_string             header_val_type;
    typedef std::pair<header_key_type, header_val_type>
                                    header_kvpair_type;
    typedef std::map<header_key_type, header_val_type>
                                    header_dict_type;

    const t_version     DEFAULT_HTTP_VERSION = V_1_1;
    const std::string   version_str(HTTP::t_version version);
    const std::string   reason(HTTP::t_status status);
}

#endif
