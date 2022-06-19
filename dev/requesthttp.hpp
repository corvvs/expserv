#ifndef REQUESTHTTP_HPP
# define REQUESTHTTP_HPP
# include <iostream>
# include <vector>
# include <map>
# include <string>
# include <list>
# include <utility>
# include <sstream>
# include "test_common.hpp"
# include "http.hpp"
# include "http_error.hpp"
# include "parserhelper.hpp"
# include "lightstring.hpp"
# include "HeaderHTTPItem.hpp"

enum t_http_request_parse_progress {
    // 開始行の開始位置 を探している
    PARSE_REQUEST_REQLINE_START,
    // 開始行の終了位置 を探している
    PARSE_REQUEST_REQLINE_END,
    // ヘッダの終了位置 を探している
    PARSE_REQUEST_HEADER_SECTION_END,
    // ヘッダ を探している
//     PARSE_REQUEST_HEADER,
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
    typedef HTTP::byte_string               byte_string;
    // 他の byte_string の一部分を参照する軽量 string
    typedef LightString<HTTP::byte_type>    light_string;
    typedef std::map<byte_string, light_string>
                                            header_dict_type;
    typedef HeaderHTTPItem::header_val_type header_val_type;


    struct ParserStatus {
        // ヘッダ終端探索時において, 最後に遭遇したCRLFのレンジ
        IndexRange                  crlf_in_header;
        // ヘッダ行解析において obs-fold に遭遇したかどうか
        bool                        found_obs_fold;

        ParserStatus():
            found_obs_fold(false) {}
    };

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

    // 解析中の情報
    ParserStatus                    ps;


    // 確定した情報
    HTTP::t_method                  http_method;
    byte_string                     request_path;
    HTTP::t_version                 http_version;
    // -1 は未指定をあらわす
    size_t                          content_length;

    // [HTTPヘッダ]
    HeaderHTTPHolder                header_holder;
    byte_string                     header_host;

    // [パース関数群]

    // 開始行の開始位置を探す
    bool    seek_reqline_start(size_t len);
    // 開始行の終了位置を探す
    bool    seek_reqline_end(size_t len);
    // [begin, end) を要求行としてパースする
    void    parse_reqline(const light_string& line);
    // ヘッダ行全体をパースする
    void    parse_header_lines(const byte_string& bytestring, ssize_t from, ssize_t len);
    // ヘッダ行をパースする
    void    parse_header_line(const light_string& line);
    
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
