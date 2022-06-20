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

RequestHTTP::ParserStatus::ParserStatus(): found_obs_fold(false) {}

RequestHTTP::RequestHTTP():
    mid(0),
    parse_progress(PARSE_REQUEST_REQLINE_START)
{
    cp.http_method = HTTP::METHOD_UNKNOWN;
    cp.http_version = HTTP::V_UNKNOWN;
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
            mid = res.second;
            if (res.is_invalid()) {
                return ;
            }
            if (ps.crlf_in_header.second != res.first) {
                // はずれ
                ps.crlf_in_header = res;
                continue;
            }
            // あたり
            // -> [start_of_header, end_of_header) を解析する
            end_of_header = res.first;
            start_of_body = res.second;
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
            if (parsed_body_size() < cp.body_size) { return; }

            mid = cp.body_size + start_of_body;
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
    ps.crlf_in_header = IndexRange(start_of_header, start_of_header);
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

            cp.http_method = discriminate_request_method(splitted[0].begin(), splitted[0].end());
            DSOUT() << splitted[0] << " -> http_method: " << cp.http_method << std::endl;
            cp.request_path = splitted[1];
            DSOUT() << "request_path: " << cp.request_path << std::endl;
            if (splitted.size() == 3) {
                cp.http_version = discriminate_request_version(splitted[2].begin(), splitted[2].end());
            } else {
                cp.http_version = HTTP::V_0_9;
            }
            DSOUT() << splitted[2] << " -> http_version: " << cp.http_version << std::endl;
            break;
        }
        default:
            throw http_error("invalid request-line?", HTTP::STATUS_BAD_REQUEST);
    }
    DSOUT() << "* parsed reqline *" << std::endl;
    start_of_current_header = mid;
}

void    RequestHTTP::parse_header_lines(const byte_string& bytebuffer, ssize_t from, ssize_t len) {
    DSOUT()
        << "\"\"\"" << std::endl
        << byte_string(bytebuffer.begin() + from, bytebuffer.begin() + from + len)
        << std::endl
        << "\"\"\"" << std::endl;
    ssize_t movement = 0;
    while(true) {
        IndexRange res = ParserHelper::find_crlf_header_value(bytebuffer, from + movement, len - movement);
        if (res.is_invalid()) {
            break;
        }
        // DSOUT() << "FOUND: " << res << std::endl;
        // [from + movement, res.first) が1つのヘッダ
        light_string header_line(
            bytebuffer,
            from + movement,
            res.first);
        if (header_line.length() > 0) {
            // header_line が空文字列でない
            // -> ヘッダ行としてパースを試みる
            parse_header_line(header_line);
        }
        // else は起きえないはず・・・

        // from + movement = res.second
        // DSOUT() << "movement: " << movement << " -> " << res.second - from << std::endl;
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
    
    // val の obs-foldを除去し, 全体を string に変換する.
    // obs-fold を検知した場合, そのことを記録する
    byte_string sval;
    {
        byte_string pval = val.str();
        ssize_t movement = 0;
        while (true) {
            IndexRange res = ParserHelper::find_obs_fold(pval, movement, val.length() - movement);
            if (res.is_invalid()) {
                sval.append(pval, movement, res.second - movement);
                break;
            }
            sval.append(pval, movement, res.first - movement);
            sval.append(ParserHelper::SP);
            DSOUT() << "found obs-fold: " << res << std::endl;
            ps.found_obs_fold = true;
            movement = res.second;
        }
        // DSOUT() << "sval: \"" << sval << "\"" << std::endl;
    }

    DSOUT()
        << "* splitted a header *"
        << std::endl
        << "Key: " << key
        << std::endl
        << "Val: " << sval
        << std::endl;
    header_holder.add_item(key, sval);
}

void    RequestHTTP::extract_control_headers() {
    // 取得したヘッダから制御用の情報を抽出する.
    // TODO: ここで何を抽出すべきか洗い出す

    // [host]
    cp.determine_host(header_holder);
    // [bodyの長さ]
    cp.determine_body_size(header_holder);

    cp.determine_content_type(header_holder);
}

void    RequestHTTP::ControlParams::determine_host(const HeaderHTTPHolder& holder) {
    // https://triple-underscore.github.io/RFC7230-ja.html#header.host
    const HeaderHTTPHolder::value_list_type *hosts = holder.get_vals(HeaderHTTP::host);
    if (!hosts || hosts->size() == 0) {
        // HTTP/1.1 なのに host がない場合, BadRequest を出す
        if (http_version == HTTP::V_1_1) {
            throw http_error("no host for HTTP/1.1", HTTP::STATUS_BAD_REQUEST);
        }
        header_host = ""; // no host
    } else if (hosts->size() > 1) {
        // Hostが複数ある場合, BadRequest を出す
        throw http_error("multiple hosts", HTTP::STATUS_BAD_REQUEST);
    } else {
        // TODO: Hostの値のバリデーション (と, 必要ならBadRequest)
        HTTP::light_string lhost(hosts->front());
        DSOUT() << "lhost: " << lhost << std::endl;
        if (!HTTP::Validator::is_valid_header_host(lhost)) {
            throw http_error("host is not valid", HTTP::STATUS_BAD_REQUEST);
        }
    }
}

void    RequestHTTP::ControlParams::determine_body_size(const HeaderHTTPHolder& holder) {
    // https://datatracker.ietf.org/doc/html/rfc7230#section-3.3.3
    const HeaderHTTPHolder::value_list_type *transfer_encodings = holder.get_vals(HeaderHTTP::transfer_encoding);
    if (transfer_encodings && transfer_encodings->back() == "chunked") {
        // メッセージのヘッダにTransfer-Encodingが存在し, かつ一番最後のcodingがchunkedである場合
        // -> chunkによって長さが決まる
        DXOUT("by chunk");
        return;
    }

    if (!transfer_encodings) {
        // Transfer-Encodingがなく, Content-Lengthが不正である時
        // -> 回復不可能なエラー
        const byte_string *cl = holder.get_val(HeaderHTTP::content_length);
        // Transfer-Encodingがなく, Content-Lengthが正当である場合
        // -> Content-Length の値がボディの長さとなる.
        if (cl) {
            body_size = ParserHelper::stou(*cl);
            // content-length の値が妥当でない場合, ここで例外が飛ぶ
            DXOUT("body_size = " << body_size);
            return;
        }
    }

    // ボディの長さは0.
    body_size = 0;
    DXOUT("body_size is zero.");
}

void    RequestHTTP::ControlParams::determine_content_type(const HeaderHTTPHolder& holder) {
    // A sender that generates a message containing a payload body SHOULD generate a Content-Type header field in that message unless the intended media type of the enclosed representation is unknown to the sender. If a Content-Type header field is not present, the recipient MAY either assume a media type of "application/octet-stream" ([RFC2046], Section 4.5.1) or examine the data to determine its type.
    //
    // Content-Type = media-type
    // media-type = type "/" subtype *( OWS ";" OWS parameter )
    // type       = token
    // subtype    = token
    // token      = 1*tchar
    // parameter  = token "=" ( token / quoted-string )

    const byte_string *ct = holder.get_val(HeaderHTTP::content_type);
    if (!ct || *ct == "") {
        content_type = "applciation/octet-stream";
        DXOUT("Content-Type -> \"" << content_type << "\"");
        return;
    }
    const light_string lct(*ct);
    light_string::size_type type_end = lct.find_first_not_of(HTTP::Charset::tchar);
    if (type_end == light_string::npos) {
        DXOUT("[KO] no /: \"" << lct << "\"");
        return;
    }
    if (lct[type_end] != '/') {
        DXOUT("[KO] not separated by /: \"" << lct << "\"");
        return;
    }
    light_string    type_str(lct, 0, type_end);
    light_string::size_type subtype_end = lct.find_first_not_of(HTTP::Charset::tchar, type_end + 1);
    DXOUT(type_end + 1 << ", " << subtype_end);
    light_string    subtype_str(lct, type_end + 1, subtype_end);
    if (subtype_str.size() == 0) {
        DXOUT("[KO] no subtype after /: \"" << lct << "\"");
        return;
    }

    if (subtype_end == light_string::npos) {
        return;
    }
    // parameters があるかも？
    DXOUT("parameters?");
    // TODO: コロン区切りリストを分解する
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
    return cp.http_version;
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
