#include "ControlHeaderHTTP.hpp"

void    HTTP::Term::TransferCoding::store_list_item(const parameter_key_type& key, const parameter_value_type& val) {
    parameters[key] = val;
}



const HTTP::byte_string  HTTP::CH::ContentType::default_value = "application/octet-stream";

void    HTTP::CH::ContentType::store_list_item(const parameter_key_type& key, const parameter_value_type& val) {
    parameters[key] = val;
}
