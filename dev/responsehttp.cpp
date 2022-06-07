#include "responsehttp.hpp"

ResponseHTTP::ResponseHTTP(
    HTTP::t_version version,
    HTTP::t_status status
):  version_(version),
    status_(status),
    is_error(false),
    consumed(0)
{}

ResponseHTTP::ResponseHTTP(
    HTTP::t_version version,
    http_error error
):  version_(version),
    status_(error.get_status()),
    is_error(true),
    consumed(0)
{}

void    ResponseHTTP::feed_header(
    const HTTP::header_key_type& key,
    const HTTP::header_val_type& val
) {
    if (header_dict.find(key) != header_dict.end() && !is_error) {
        throw std::runtime_error("Invalid Response: Duplicate header");
    }
    header_dict[key] = val;
    header_list.push_back(HTTP::header_kvpair_type(key, val));
}

void    ResponseHTTP::feed_body(const byte_string& str) {
    body = str;
}

void    ResponseHTTP::feed_body(byte_string::const_iterator first, byte_string::const_iterator last) {
    body = byte_string(first, last);
}

void    ResponseHTTP::render() {
    // 状態行
    message_text =
        HTTP::version_str(version_) + ParserHelper::SP +
        ParserHelper::utos(status_) + ParserHelper::SP +
        HTTP::reason(status_) + ParserHelper::CRLF;
    // content-length がない場合, かつbodyをもつメソッドの場合(TODO),
    // bodyのサイズをcontent-lengthとして出力
    if (header_dict.find("content-length") == header_dict.end()) {
        header_list.insert(
            header_list.begin(),
            HTTP::header_kvpair_type(
                "content-length",
                ParserHelper::utos(body.length())
            )
        );
    }
    // ヘッダ
    for (
        std::vector<HTTP::header_kvpair_type>::iterator it = header_list.begin();
        it != header_list.end();
        it++
    ) {
        message_text +=
            it->first + ParserHelper::HEADER_KV_SPLITTER +
            it->second + ParserHelper::CRLF;
    }
    // 空行
    message_text += ParserHelper::CRLF;
    // ボディ
    message_text += body;
}

const ResponseHTTP::byte_string&  ResponseHTTP::get_message_text() const {
    return message_text;
}

const char* ResponseHTTP::get_unsent() const {
    return message_text.c_str() + consumed;
}

void        ResponseHTTP::mark_sent(size_t sent) {
    consumed += sent;
    if (message_text.length() < consumed) {
        consumed = message_text.length();
    }
}

size_t      ResponseHTTP::get_unsent_size() const {
    return message_text.length() - consumed;
}
