#ifndef REQUESTHTTP_HPP
# define REQUESTHTTP_HPP
# include <iostream>
# include <vector>
# include <map>
# include <string>
# include <utility>
# include <sstream>
# include "test_common.hpp"
# include "http.hpp"
# include "http_error.hpp"
# include "parserhelper.hpp"

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

// [HTTPリクエストクラス]
// [責務]
// - 順次供給されるバイト列をHTTPリクエストとして解釈すること
class RequestHTTP {
public:
    static const size_t MAX_REQLINE_END = 8192;
    typedef HTTP::byte_string       byte_string;
    typedef HTTP::header_dict_type  header_dict_type;

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
    HTTP::t_method                  http_method;
    byte_string                     request_path;
    HTTP::t_version                 http_version;
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
    void    feed_bytestring(char *bytes, size_t len);

    // 受信済み(未解釈含む)データサイズ
    size_t  receipt_size() const;
    // 解釈済みボディサイズ
    size_t  parsed_body_size() const;
    // 解釈済みデータサイズ
    size_t  parsed_size() const;

    // リクエストのHTTPバージョン
    HTTP::t_version
            get_http_version() const;

    // 指定した key のヘッダーを取得する
    // - first: そのヘッダーが存在するかどうか
    // - second: そのヘッダーの value; 存在しない場合は空文字列
    std::pair<bool, byte_string>
            get_header(const byte_string& header_key) const;

    // リクエスト本文の開始位置
    byte_string::const_iterator
            get_body_begin() const;
    // リクエスト本文の終了位置
    byte_string::const_iterator
            get_body_end() const;

    // predicate: ナビゲーション(ルーティング)できる状態になったかどうか
    bool    is_ready_to_navigate() const;
    // predicate: レスポンスを作成できる状態になったかどうか
    bool    is_ready_to_respond() const;
    // predicate: このリクエストに対するレスポンスを送り終わった後, 接続を維持すべきかどうか
    bool    should_keep_in_touch() const;
};

#endif
