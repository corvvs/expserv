#ifndef REQUESTHTTP_HPP
# define REQUESTHTTP_HPP
# include <iostream>
# include <vector>
# include <string>
# include <utility>
# include "test_common.hpp"
# include "parserhelper.hpp"

enum t_http_version {
    HTTP_V_UNKNOWN,
    HTTP_V_0_9,
    HTTP_V_1_0,
    HTTP_V_1_1,
    HTTP_V_ERROR
};

enum t_http_method {
    HTTP_METHOD_UNKNOWN,
    HTTP_METHOD_GET,
    HTTP_METHOD_POST,
    HTTP_METHOD_DELETE,
    HTTP_METHOD_ERROR
};

enum t_http_request_parse_progress {
    // 開始行の開始位置 を探している
    PARSE_REQUEST_REQLINE_START,
    // 開始行の終了位置 を探している
    PARSE_REQUEST_REQLINE_END,
    // ヘッダ を探している
    PARSE_REQUEST_HEADER,
    // ボディ を探している
    PARSE_REQUEST_BODY,
    PARSE_REQUEST_OVER,
    PARSE_REQUEST_ERROR
};

class RequestHTTP {
public:
    typedef std::basic_string<char> byte_buffer;
private:
    byte_buffer                     bytebuffer;
    ssize_t                         mid;

    // パースに関する情報
    t_http_request_parse_progress   parse_progress;
    size_t                          start_of_reqline;
    size_t                          end_of_reqline;
    size_t                          start_of_header;
    size_t                          end_of_header;
    size_t                          start_of_body;
    // size_t                          end_of_body;

    byte_buffer                     raw_header;

    // 開始行の開始位置を探す
    bool    extract_reqline_start(size_t len);

    // 開始行の終了位置を探す
    bool    extract_reqline_end(size_t len);

    // [begin, end) を要求行としてパースする
    void    parse_reqline();

    bool    extract_header_end(size_t len);


    // 確定した情報
    t_http_method                   http_method;
    std::string                     request_path;
    t_http_version                  http_version;

public:
    RequestHTTP();
    ~RequestHTTP();

    // 内部バッファにバイト列を追加し, ラフパースを試みる
    void    feed_bytes(char *bytes, size_t len);
};

#endif
