#ifndef CONTROLHEADERHTTP_HPP
# define CONTROLHEADERHTTP_HPP
# include "http.hpp"
# include "lightstring.hpp"
# include <map>

namespace HTTP {
    // Control-Header
    namespace CH {
        // Host
        struct Host {
            HTTP::byte_string   port;
            HTTP::byte_string   host;
        };

        // Content-Type
        struct ContentType {
            typedef std::map<std::string, HTTP::light_string> parameter_dict;

            HTTP::byte_string   value;
            parameter_dict      parameters;
        };
    }
}

#endif
