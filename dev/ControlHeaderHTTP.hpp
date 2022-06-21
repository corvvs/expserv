#ifndef CONTROLHEADERHTTP_HPP
# define CONTROLHEADERHTTP_HPP
# include "http.hpp"
# include "lightstring.hpp"
# include <map>

// Content-Type
struct CHContentType {
    typedef std::map<std::string, HTTP::light_string> parameter_dict;

    HTTP::byte_string   value;
    parameter_dict      parameters;
};


#endif
