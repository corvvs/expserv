#ifndef VALIDATORHTTP_HPP
# define VALIDATORHTTP_HPP
# include "http.hpp"
# include "LightString.hpp"
# include <string>
# include <list>
# include <map>

namespace HTTP {
    namespace Validator {
        bool    is_valid_header_host(const HTTP::light_string& str);
        // URI
        bool    is_uri_host(const HTTP::light_string& str);
        bool    is_ip_literal(const HTTP::light_string& str);
        bool    is_ipv6address(const HTTP::light_string& str);
        bool    is_ipvfuture(const HTTP::light_string& str);
        bool    is_ipv4address(const HTTP::light_string& str);
        bool    is_reg_name(const HTTP::light_string& str);
        bool    is_port(const HTTP::light_string& str);
        bool    is_h16(const HTTP::light_string& str);
        bool    is_ls32(const HTTP::light_string& str);
    }
}

#endif
