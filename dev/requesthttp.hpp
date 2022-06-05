#ifndef REQUESTHTTP_HPP
# define REQUESTHTTP_HPP
# include <iostream>
# include <vector>
# include <map>
# include <string>
# include <utility>
# include <sstream>
# include "test_common.hpp"
# include "http_error.hpp"
# include "parserhelper.hpp"

enum t_http_method {
    HTTP_METHOD_UNKNOWN,
    HTTP_METHOD_GET,
    HTTP_METHOD_POST,
    HTTP_METHOD_DELETE,
    HTTP_METHOD_ERROR
};

enum t_http_version {
    HTTP_V_UNKNOWN,
    HTTP_V_0_9,
    HTTP_V_1_0,
    HTTP_V_1_1,
    HTTP_V_ERROR
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
    static const size_t MAX_REQLINE_END = 8192;
    typedef std::basic_string<char>
                                    byte_string;
    typedef std::map< byte_string, byte_string >
                                    header_dict_type;

private:
    byte_string                     bytebuffer;
    ssize_t                         mid;

    // パースに関する情報
    t_http_request_parse_progress   parse_progress;
    size_t                          start_of_reqline;
    size_t                          end_of_reqline;
    size_t                          start_of_current_header;
    size_t                          start_of_header;
    size_t                          end_of_header;
    size_t                          start_of_body;
    // size_t                          end_of_body;

    byte_string                     raw_header;

    // 確定した情報
    t_http_method                   http_method;
    std::string                     request_path;
    t_http_version                  http_version;
    // -1 は未指定をあらわす
    size_t                          content_length;

    // [HTTPヘッダ]

    // HTTPヘッダのキーが登場順に入る
    std::vector< byte_string >      header_keys;
    // HTTPヘッダのキーと値
    header_dict_type                header_dict;

    byte_string                     header_host;


    // 開始行の開始位置を探す
    bool    extract_reqline_start(size_t len);
    // 開始行の終了位置を探す
    bool    extract_reqline_end(size_t len);
    // [begin, end) を要求行としてパースする
    void    parse_reqline(const byte_string& header_line);
    // 
    bool    extract_header_end(size_t len);
    // ヘッダ行をパースする
    void    parse_header_line(const byte_string& header_line);
    // ヘッダから必要な情報を取る
    void    extract_control_headers();



public:
    RequestHTTP();
    ~RequestHTTP();

    // 内部バッファにバイト列を追加し, ラフパースを試みる
    void    feed_bytes(char *bytes, size_t len);

    // ナビゲーション(ルーティング)できる状態になったかどうか
    bool    navigation_ready() const;
    // レスポンスを作成できる状態になったかどうか
    bool    respond_ready() const;
    // 受信済み(未解釈含む)データサイズ
    size_t  receipt_size() const;
    // 解釈済みボディサイズ
    size_t  parsed_body_size() const;
    // 解釈済みデータサイズ
    size_t  parsed_size() const;

    std::pair<bool, byte_string> get_header(const byte_string& header_key) const;
};

#endif
