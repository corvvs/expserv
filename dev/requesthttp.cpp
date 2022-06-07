#include "requesthttp.hpp"

HTTP::t_method   judge_http_method(
    RequestHTTP::byte_string::iterator begin,
    RequestHTTP::byte_string::iterator end
) {
    RequestHTTP::byte_string sub(begin, end);
    if (sub == "GET") {
        return HTTP::METHOD_GET;
    }
    if (sub == "POST") {
        return HTTP::METHOD_POST;
    }
    if (sub == "DELETE") {
        return HTTP::METHOD_DELETE;
    }
    throw http_error("Invalid Request: unsupported method", HTTP::STATUS_METHOD_NOT_ALLOWED);
}

HTTP::t_version  judge_http_version(
    RequestHTTP::byte_string::iterator begin,
    RequestHTTP::byte_string::iterator end
) {
    RequestHTTP::byte_string sub(begin, end);
    if (sub == HTTP::version_str(HTTP::V_1_0)) {
        return HTTP::V_1_0;
    }
    if (sub == HTTP::version_str(HTTP::V_1_1)) {
        return HTTP::V_1_1;
    }
    throw http_error("Invalid Request: unsupported version", HTTP::STATUS_VERSION_NOT_SUPPORTED);
}

RequestHTTP::RequestHTTP():
    mid(0),
    parse_progress(PARSE_REQUEST_REQLINE_START),
    http_method(HTTP::METHOD_UNKNOWN),
    http_version(HTTP::V_UNKNOWN)
{
    bytebuffer.reserve(MAX_REQLINE_END);
    std::cout << "[R] generated" << std::endl;
}

RequestHTTP::~RequestHTTP() {}

void    RequestHTTP::feed_bytes(char *bytes, size_t len) {
    bytebuffer.insert(bytebuffer.end(), bytes, bytes + len);
    std::cout << "[R] feed: " << len << std::endl;
    std::cout << "[R] progress: " << parse_progress << std::endl;
    std::cout
        << "```" << std::endl
        << bytebuffer
        << "```" << std::endl;

    for (;len > 0; ) {
        len = bytebuffer.length() - mid;
        DSOUT() << "mid: " << mid <<  ", len: " << len << std::endl;
        DSOUT() << "parsed: " << parsed_size() << ", receipt: " << receipt_size() << std::endl;
        switch (parse_progress) {
            // 開始行の開始位置を探す
            case PARSE_REQUEST_REQLINE_START: {
                if (!extract_reqline_start(len)) {
                    return;
                }
                parse_progress = PARSE_REQUEST_REQLINE_END;
                continue;
            }

            // 開始行の終了位置を探す
            case PARSE_REQUEST_REQLINE_END: {
                if (!extract_reqline_end(len)) {
                    return;
                }
                byte_string raw_req_line(
                    bytebuffer.begin() + start_of_reqline,
                    bytebuffer.begin() + end_of_reqline);
                // -> [start_of_reqline, end_of_reqline) が 開始行かどうか調べる.
                parse_reqline(raw_req_line);
                parse_progress = PARSE_REQUEST_HEADER;
                continue;
            }

            // ヘッダの終わりを探す
            case PARSE_REQUEST_HEADER: {
                DSOUT() << "* parsing header lines... *" << std::endl;
                // DSOUT() << "end_of_reqline: " << end_of_reqline << std::endl;
                // DSOUT() << "start_of_current_header: " << start_of_current_header << std::endl;
                // DSOUT() << "mid: " << mid << std::endl;
                // DSOUT() << "len: " << len << std::endl;
                ParserHelper::index_range res = ParserHelper::find_crlf(bytebuffer, mid, len);
                // DSOUT() << "[" << res.first << "," << res.second << ")" << std::endl;
                
                mid += res.second;
                if (res.first >= res.second) {
                    return ;
                }
                // CRLFが見つかった
                // -> [mid, res.first) が1つのヘッダ
                // DSOUT() << "[" << start_of_current_header << "," << mid - (res.second - res.first) << ")" << std::endl;
                byte_string header_line(
                    bytebuffer.begin() + start_of_current_header,
                    bytebuffer.begin() + mid - (res.second - res.first));
                if (header_line.length() > 0) {
                    // header_line が空文字列でない
                    // -> ヘッダとしてパースを試みる
                    parse_header_line(header_line);
                    continue;
                }
                // header_line が空文字列
                // -> 空行が見つかったのでそこでヘッダが終わる
                end_of_header = mid - (res.second - res.first);
                DSOUT() << "* determined end_of_header *" << std::endl;
                DSOUT() << "end_of_header: " << end_of_header << std::endl;
                raw_header = byte_string(
                    bytebuffer.begin() + start_of_header,
                    bytebuffer.begin() + end_of_header);
                extract_control_headers();
                start_of_body = mid;

                parse_progress = PARSE_REQUEST_BODY;
                continue;
            }

            case PARSE_REQUEST_BODY: {
                // TODO: 切断対応
                mid += len;
                DSOUT() << "parsed_body_size: " << parsed_body_size() << std::endl;
                DSOUT() << "start_of_body: " << start_of_body << std::endl;
                DSOUT() << "content_length: " << content_length << std::endl;
                // - 接続が切れていたら
                //   -> ここまでをbodyとする
                // - content-lengthが不定なら
                //   -> 続行
                // - 受信済みサイズが content-length を下回っていたら
                //   -> 続行
                // - (else) 受信済みサイズが content-length 以上なら
                //   -> 受信済みサイズ = mid - start_of_body が content-length になるよう調整
                if (parsed_body_size() < content_length) {
                    return;
                }

                mid = content_length + start_of_body;
                parse_progress = PARSE_REQUEST_OVER;
            }

            case PARSE_REQUEST_OVER: {
                DSOUT() << "FINISH" << std::endl;
                return;
            }

            default:
                throw http_error("not implemented yet", HTTP::STATUS_NOT_IMPLEMENTED);
        }
    }
}

bool    RequestHTTP::extract_reqline_start(size_t len) {
    DSOUT() << "* determining start_of_reqline... *" << std::endl;
    size_t s_o_s = ParserHelper::ignore_crlf(bytebuffer, mid, len);
    DOUT() << len << " -> " << s_o_s << std::endl;
    mid += s_o_s;
    if (s_o_s == len) {
        return false;
    }
    // 開始行の開始位置が定まった
    DSOUT() << "* determined start_of_reqline *" << std::endl;
    start_of_reqline = mid;
    DOUT() << "start_of_reqline is: " << start_of_reqline << std::endl;
    return true;
}

bool    RequestHTTP::extract_reqline_end(size_t len) {
    DSOUT() << "* determining end_of_reqline... *" << std::endl;
    ParserHelper::index_range res = ParserHelper::find_crlf(bytebuffer, mid, len);
    DSOUT() << "[" << res.first << "," << res.second << ")" << std::endl;
    mid += res.second;
    if (res.first >= res.second) { return false; }
    // CRLFが見つかった
    end_of_reqline = mid - (res.second - res.first);
    start_of_header = mid;
    // -> end_of_reqline が8192バイト以内かどうか調べる。
    if (MAX_REQLINE_END <= end_of_reqline) {
        throw http_error("Invalid Response: request line is too long", HTTP::STATUS_URI_TOO_LONG);
    }
    return true;
}

void    RequestHTTP::parse_reqline(const byte_string& raw_req_line) {
    DSOUT() << "* determined end_of_reqline *" << std::endl;
    DSOUT() << "end_of_reqline: " << end_of_reqline << std::endl;
    DSOUT() << "start_of_header: " << start_of_header << std::endl;
    DSOUT() << "* parsing reqline... *" << std::endl;
    DSOUT() << "\"" << raw_req_line << "\"" << std::endl;

    std::vector< byte_string > splitted = ParserHelper::split_by_sp(raw_req_line.begin(), raw_req_line.end());

    switch (splitted.size()) {
        case 2:
        case 3: {
            // HTTP/0.9?
            // HTTP/1.*?

            http_method = judge_http_method(splitted[0].begin(), splitted[0].end());
            DSOUT() << byte_string(splitted[0].begin(), splitted[0].end()) << " -> http_method: " << http_method << std::endl;
            request_path = splitted[1];
            DSOUT() << "request_path: " << request_path << std::endl;
            DSOUT() << "\"" << byte_string(splitted[2].begin(), splitted[2].end()) << "\"" << std::endl;
            if (splitted.size() == 3) {
                http_version = judge_http_version(splitted[2].begin(), splitted[2].end());
            } else {
                http_version = HTTP::V_0_9;
            }
            DSOUT() << "http_version: " << http_version << std::endl;
            break;
        }
        default:
            throw std::runtime_error("Invalid Request: invalid request-line?");
    }
    DSOUT() << "* parsed reqline *" << std::endl;
    start_of_current_header = mid;
}

bool    RequestHTTP::extract_header_end(size_t len) {
    ParserHelper::index_range res = ParserHelper::find_blank_line(bytebuffer, mid, len);
    DSOUT() << "res: [" << res.first << "," << res.second << ")" << std::endl;
    mid += res.second;
    if (res.first >= res.second) { return false; }
    end_of_header = mid - (res.second - res.first);
    start_of_body = mid;
    return true;
}

void    RequestHTTP::parse_header_line(const byte_string& header_line) {
    // ヘッダを解析する
    start_of_current_header = mid;
    DSOUT()
        << "found a header: "
        << std::endl
        << header_line
        << std::endl;

    byte_string::size_type  coron_pos = header_line.find_first_of(ParserHelper::HEADER_KV_SPLITTER);
    if (coron_pos == byte_string::npos) {
        // ":"がない
        // -> おかしなヘッダ
        // [!] Apache は : が含まれず空白から始まらない行がヘッダー部にあると、 400 応答を返します。 nginx は無視して処理を続けます。
        throw http_error("Invalid Request: header does not contain a coron", HTTP::STATUS_BAD_REQUEST);
    }
    // ":"があった
    // -> ":"の前後をキーとバリューにする
    byte_string header_key(header_line.begin(), header_line.begin() + coron_pos);
    byte_string header_value(header_line.begin() + coron_pos + 1, header_line.end());
    // [!] 欄名と : の間には空白は認められていません。 鯖は、空白がある場合 400 応答を返して拒絶しなければなりません。 串は、下流に転送する前に空白を削除しなければなりません。
    if (header_key.length() == 0) {
        throw http_error("Invalid Request: header key is empty", HTTP::STATUS_BAD_REQUEST);
    }
    byte_string::size_type header_key_tail = header_key.find_last_not_of(ParserHelper::OWS);
    if (header_key_tail + 1 != header_key.length()) {
        throw http_error("Invalid Request: found space between header key and coron", HTTP::STATUS_BAD_REQUEST);
    }
    // [!] 欄値の前後の OWS は、欄値の一部ではなく、 構文解析の際に削除します
    byte_string::size_type header_value_head = header_value.find_first_not_of(ParserHelper::OWS);
    byte_string::size_type header_value_tail = header_value.find_last_not_of(ParserHelper::OWS);
    if (header_value_head == header_value_tail) {
        // Valueが空 -> エラーではない
        header_value = "";
    } else {
        header_value = header_value.substr(header_value_head, header_value_tail + 1);
    }
    

    // Keyの正規化
    byte_string normalized_header_key(header_key);
    for (byte_string::iterator it = normalized_header_key.begin(); it != normalized_header_key.end(); it++) {
        *it = tolower(*it);
    }
    std::pair<std::map< byte_string, byte_string >::iterator, bool> res_insertion = header_dict.insert(std::pair<byte_string, byte_string>(normalized_header_key, header_value));
    if (res_insertion.second) {
        header_keys.push_back(normalized_header_key);
    }
    DSOUT()
        << "* splitted a header *"
        << std::endl
        << "Key: " << header_key
        << std::endl
        << "Val: " << header_value
        << std::endl;
}

void    RequestHTTP::extract_control_headers() {
    // - host
    header_host = header_dict["host"];
    if (header_host.length() == 0) {
        // host が空
        // 1.1ならbad request
        throw http_error("Invalid Request: no host", HTTP::STATUS_BAD_REQUEST);
    }
    DSOUT() << "host is: " << header_host << std::endl;
    // - content-length
    // bodyを持たないメソッドの場合は0
    // そうでない場合, content-length の値を変換する
    switch (http_method) {
        case HTTP::METHOD_GET:
        case HTTP::METHOD_DELETE: {
            content_length = 0;
            break;
        }
        default: {
            byte_string cl = header_dict["content-length"];
            if (cl == "") {
                content_length = -1;
            } else {
                content_length = ParserHelper::stou(cl);
            }
        }
    }
    DSOUT() << "content-length is: " << content_length << std::endl;
}

bool    RequestHTTP::navigation_ready() const {
    return parse_progress >= PARSE_REQUEST_BODY;
}

bool    RequestHTTP::respond_ready() const {
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

std::pair<bool, RequestHTTP::byte_string>    RequestHTTP::get_header(const byte_string& header_key) const {
    header_dict_type::const_iterator result = header_dict.find(header_key);
    if (result == header_dict.end()) {
        return std::pair<bool, RequestHTTP::byte_string>(false, "");
    }
    return std::pair<bool, RequestHTTP::byte_string>(true, result->second);
}

RequestHTTP::byte_string::const_iterator  RequestHTTP::get_body_begin() const {
    return bytebuffer.begin() + start_of_body;
}

RequestHTTP::byte_string::const_iterator  RequestHTTP::get_body_end() const {
    return bytebuffer.end();
}
