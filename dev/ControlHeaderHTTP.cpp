#include "ControlHeaderHTTP.hpp"

void    HTTP::Term::TransferCoding::store_list_item(const parameter_key_type& key, const parameter_value_type& val) {
    parameters[key] = val;
}



bool    HTTP::CH::TransferEncoding::empty() const {
    return tranfer_codings.empty();
}

const HTTP::Term::TransferCoding&
    HTTP::CH::TransferEncoding::current_coding() const {
    return tranfer_codings.back();
}

const HTTP::byte_string  HTTP::CH::ContentType::default_value = "application/octet-stream";

void
    HTTP::CH::ContentType::store_list_item(const parameter_key_type& key, const parameter_value_type& val) {
    parameters[key] = val;
}

bool
    HTTP::CH::Connection::will_close() const {
    return close_;
}

bool
    HTTP::CH::Connection::will_keep_alive() const {
    return !close_ && keep_alive_;
}

