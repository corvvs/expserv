#include "requesthttp.hpp"

HTTP::t_method   discriminate_request_method(
    RequestHTTP::byte_string::iterator begin,
    RequestHTTP::byte_string::iterator end
) {
    RequestHTTP::byte_string sub(begin, end);
    if (sub == "GET")       { return HTTP::METHOD_GET; }
    if (sub == "POST")      { return HTTP::METHOD_POST; }
    if (sub == "DELETE")    { return HTTP::METHOD_DELETE; }
    throw http_error("unsupported method", HTTP::STATUS_METHOD_NOT_ALLOWED);
}

HTTP::t_version  discriminate_request_version(
    RequestHTTP::byte_string::iterator begin,
    RequestHTTP::byte_string::iterator end
) {
    RequestHTTP::byte_string sub(begin, end);
    if (sub == HTTP::version_str(HTTP::V_1_0)) { return HTTP::V_1_0; }
    if (sub == HTTP::version_str(HTTP::V_1_1)) { return HTTP::V_1_1; }
    throw http_error("unsupported version", HTTP::STATUS_VERSION_NOT_SUPPORTED);
}

RequestHTTP::RequestHTTP():
    mid(0),
    parse_progress(PARSE_REQUEST_REQLINE_START),
    http_method(HTTP::METHOD_UNKNOWN),
    http_version(HTTP::V_UNKNOWN)
{
    bytebuffer.reserve(MAX_REQLINE_END);
}

RequestHTTP::~RequestHTTP() {}

void    RequestHTTP::feed_bytestring(char *bytes, size_t len) {
    bytebuffer.insert(bytebuffer.end(), bytes, bytes + len);

    for (;len > 0;) {
        len = bytebuffer.length() - mid;

        switch (parse_progress) {
        case PARSE_REQUEST_REQLINE_START: {

            // 開始行の開始位置を探す
            if (!seek_reqline_start(len)) { return; }
            parse_progress = PARSE_REQUEST_REQLINE_END;

            continue;
        }

        case PARSE_REQUEST_REQLINE_END: {

            // 開始行の終了位置を探す
            if (!seek_reqline_end(len)) { return; }
            light_string raw_req_line(
                bytebuffer,
                start_of_reqline,
                end_of_reqline);
            // -> [start_of_reqline, end_of_reqline) が 開始行かどうか調べる.
            parse_reqline(raw_req_line);
            parse_progress = PARSE_REQUEST_HEADER_SECTION_END;

            continue;
        }

        case PARSE_REQUEST_HEADER_SECTION_END: {
            // ヘッダ部の終わりを探索する
            // `crlf_in_header` には「最後に見つけたCRLF」のレンジが入っている.
            // mid以降についてCRLFを検索する:
            // - 見つかった
            //   - first == crlf_in_header.second である
            //     -> あたり. このCRLFがヘッダの終わりを示す.
            //   - そうでない
            //     -> はずれ. このCRLFを crlf_in_header として続行.
            // - 見つからなかった
            //   -> もう一度受信する
            IndexRange res = ParserHelper::find_crlf(bytebuffer, mid, len);
            DSOUT() << res << std::endl;
            mid = res.second;
            if (res.is_invalid()) {
                return ;
            }
            DSOUT() << crlf_in_header << std::endl;
            if (crlf_in_header.second != res.first) {
                // はずれ
                crlf_in_header = res;
                continue;
            }
            // あたり
            DSOUT() << "HIT!!" << std::endl;
            end_of_header = res.first;
            start_of_body = res.second;
            DSOUT()
                << "\"\"\"" << std::endl
                << byte_string(bytebuffer.begin() + start_of_header, bytebuffer.begin() + end_of_header)
                << std::endl
                << "\"\"\"" << std::endl;
            // [start_of_header, end_of_header) を解析する
            parse_header_lines(bytebuffer, start_of_header, end_of_header - start_of_header);
            extract_control_headers();
            parse_progress = PARSE_REQUEST_BODY;
            continue;
        }

        case PARSE_REQUEST_BODY: {

            // TODO: 切断対応
            mid += len;
            // DSOUT() << "parsed_body_size: " << parsed_body_size() << std::endl;
            // DSOUT() << "start_of_body: " << start_of_body << std::endl;
            // DSOUT() << "content_length: " << content_length << std::endl;
            // - 接続が切れていたら
            //   -> ここまでをbodyとする
            // - content-lengthが不定なら
            //   -> 続行
            // - 受信済みサイズが content-length を下回っていたら
            //   -> 続行
            // - (else) 受信済みサイズが content-length 以上なら
            //   -> 受信済みサイズ = mid - start_of_body が content-length になるよう調整
            if (parsed_body_size() < content_length) { return; }

            mid = content_length + start_of_body;
            parse_progress = PARSE_REQUEST_OVER;

            continue;
        }

        case PARSE_REQUEST_OVER: {
            return;
        }

        default:
            throw http_error("not implemented yet", HTTP::STATUS_NOT_IMPLEMENTED);
        }
    }
}

bool    RequestHTTP::seek_reqline_start(size_t len) {
    DSOUT() << "* determining start_of_reqline... *" << std::endl;
    size_t s_o_s = ParserHelper::ignore_crlf(bytebuffer, mid, len);
    mid += s_o_s;
    if (s_o_s == len) {
        return false;
    }
    // 開始行の開始位置が定まった
    start_of_reqline = mid;
    return true;
}

bool    RequestHTTP::seek_reqline_end(size_t len) {
    // DSOUT() << "* determining end_of_reqline... *" << std::endl;
    IndexRange res = ParserHelper::find_crlf(bytebuffer, mid, len);
    mid = res.second;
    if (res.is_invalid()) { return false; }
    // CRLFが見つかった
    end_of_reqline = res.first;
    start_of_header = mid;
    // -> end_of_reqline が8192バイト以内かどうか調べる。
    if (MAX_REQLINE_END <= end_of_reqline) {
        throw http_error("Invalid Response: request line is too long", HTTP::STATUS_URI_TOO_LONG);
    }
    crlf_in_header = IndexRange(start_of_header, start_of_header);
    return true;
}

void    RequestHTTP::parse_reqline(const light_string& raw_req_line) {
    // DSOUT() << "* determined end_of_reqline *" << std::endl;
    // DSOUT() << "end_of_reqline: " << end_of_reqline << std::endl;
    // DSOUT() << "start_of_header: " << start_of_header << std::endl;
    // DSOUT() << "* parsing reqline... *" << std::endl;
    // DSOUT() << "\"" << raw_req_line << "\"" << std::endl;

    std::vector< byte_string > splitted = ParserHelper::split_by_sp(raw_req_line.begin(), raw_req_line.end());

    switch (splitted.size()) {
        case 2:
        case 3: {
            // HTTP/0.9?
            // HTTP/1.*?

            http_method = discriminate_request_method(splitted[0].begin(), splitted[0].end());
            DSOUT() << splitted[0] << " -> http_method: " << http_method << std::endl;
            request_path = splitted[1];
            DSOUT() << "request_path: " << request_path << std::endl;
            if (splitted.size() == 3) {
                http_version = discriminate_request_version(splitted[2].begin(), splitted[2].end());
            } else {
                http_version = HTTP::V_0_9;
            }
            DSOUT() << splitted[2] << " -> http_version: " << http_version << std::endl;
            break;
        }
        default:
            throw http_error("invalid request-line?", HTTP::STATUS_BAD_REQUEST);
    }
    DSOUT() << "* parsed reqline *" << std::endl;
    start_of_current_header = mid;
}

void    RequestHTTP::parse_header_lines(const byte_string& bytebuffer, ssize_t from, ssize_t len) {
    ssize_t movement = 0;
    while(true) {
        IndexRange res = ParserHelper::find_crlf_header_value(bytebuffer, from + movement, len - movement);
        DSOUT() << "FOUND: " << res << std::endl;
        if (res.is_invalid()) {
            break;
        }
        // [from + movement, res.first) が1つのヘッダ
        light_string header_line(
            bytebuffer,
            from + movement,
            res.first);
        if (header_line.length() > 0) {
            // header_line が空文字列でない
            // -> ヘッダとしてパースを試みる
            parse_header_line(header_line);
        }
        // else は起きえないはず・・・

        // from + movement = res.second
        DSOUT() << "movement: " << movement << " -> " << res.second - from << std::endl;
        movement = res.second - from;
    }
}


void    RequestHTTP::parse_header_line(const light_string& line) {
    // ヘッダを解析する
    start_of_current_header = mid;

    light_string::size_type  coron_pos = line.find_first_of(ParserHelper::HEADER_KV_SPLITTER);
    if (coron_pos == byte_string::npos) {
        // [!] Apache は : が含まれず空白から始まらない行がヘッダー部にあると、 400 応答を返します。 nginx は無視して処理を続けます。
        throw http_error("no coron in a head line", HTTP::STATUS_BAD_REQUEST);
    }

    // ":"があった -> ":"の前後をキーとバリューにする
    light_string key(line, 0, coron_pos);
    if (key.length() == 0) {
        throw http_error("header key is empty", HTTP::STATUS_BAD_REQUEST);
    }
    light_string val(line, coron_pos + 1, line.length());
    // [!] 欄名と : の間には空白は認められていません。 鯖は、空白がある場合 400 応答を返して拒絶しなければなりません。 串は、下流に転送する前に空白を削除しなければなりません。
    light_string::size_type key_tail = key.find_last_not_of(ParserHelper::OWS);
    if (key_tail + 1 != key.length()) {
        throw http_error("trailing space on header key", HTTP::STATUS_BAD_REQUEST);
    }
    // [!] 欄値の前後の OWS は、欄値の一部ではなく、 構文解析の際に削除します
    light_string::size_type val_head = val.find_first_not_of(ParserHelper::OWS);
    light_string::size_type val_tail = val.find_last_not_of(ParserHelper::OWS);
    if (val_head == light_string::npos) {
        // Valueが空 -> エラーではない
        val = "";
    } else {
        val = val.substr(val_head, val_tail - val_head + 1);
    }

    // Keyの正規化; 変更するので light_string にしない
    byte_string norm_key(key.str());
    ParserHelper::normalize_header_key(norm_key);
    std::pair<header_dict_type::iterator, bool> res_insertion
        = header_dict.insert(
            header_dict_type::value_type(norm_key, val)
        );
    if (res_insertion.second) {
        header_keys.push_back(norm_key);
    }
    DSOUT()
        << "* splitted a header *"
        << std::endl
        << "Key: " << key.str()
        << std::endl
        << "Val: " << val.str()
        << std::endl;
}

void    RequestHTTP::extract_control_headers() {
    // [Host]
    {
        header_host = header_dict["host"].str();
        if (header_host.length() == 0) {
            // host が空
            // 1.1ならbad request
            throw http_error("no host", HTTP::STATUS_BAD_REQUEST);
        }
        // DSOUT() << "host is: " << header_host << std::endl;
    }

    // [Content-Length]
    {
        // bodyを持たないメソッドの場合は0
        // そうでない場合, content-length の値を変換する
        switch (http_method) {
            case HTTP::METHOD_GET:
            case HTTP::METHOD_DELETE: {
                content_length = 0;
                break;
            }
            default: {
                byte_string cl = header_dict["content-length"].str();
                if (cl == "") {
                    content_length = -1;
                } else {
                    content_length = ParserHelper::stou(cl);
                }
            }
        }
        // DSOUT() << "content-length is: " << content_length << std::endl;
    }

    // [Transfer-Encoding]
    {
        byte_string te = header_dict["transfer-encoding"].str();
        
    }
}

bool    RequestHTTP::is_ready_to_navigate() const {
    return parse_progress >= PARSE_REQUEST_BODY;
}

bool    RequestHTTP::is_ready_to_respond() const {
    return parse_progress >= PARSE_REQUEST_OVER;
}

size_t  RequestHTTP::receipt_size() const {
    return bytebuffer.length();
}

size_t  RequestHTTP::parsed_body_size() const {
    return mid - start_of_body;
}

size_t  RequestHTTP::parsed_size() const {
    return mid;
}

HTTP::t_version RequestHTTP::get_http_version() const {
    return http_version;
}

std::pair<RequestHTTP::light_string, bool>
                RequestHTTP::get_header(const byte_string& key) const {
    header_dict_type::const_iterator result = header_dict.find(key);
    if (result == header_dict.end()) {
        return std::pair<light_string, bool>(light_string(""), false);
    }
    return std::pair<light_string, bool>(result->second, true);
}

RequestHTTP::byte_string::const_iterator
                RequestHTTP::get_body_begin() const {
    return bytebuffer.begin() + start_of_body;
}

RequestHTTP::byte_string::const_iterator
                RequestHTTP::get_body_end() const {
    return bytebuffer.end();
}

bool            RequestHTTP::should_keep_in_touch() const {
    // TODO: 仮実装
    return false;
}
