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
    parse_progress(PARSE_REQUEST_REQLINE_START),
    cp()
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
    DXOUT("* determining start_of_reqline... *");
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
            DXOUT(splitted[0] << " -> http_method: " << cp.http_method);
            cp.request_path = splitted[1];
            DXOUT("request_path: " << cp.request_path);
            if (splitted.size() == 3) {
                cp.http_version = discriminate_request_version(splitted[2].begin(), splitted[2].end());
            } else {
                cp.http_version = HTTP::V_0_9;
            }
            DXOUT(splitted[2] << " -> http_version: " << cp.http_version);
            break;
        }
        default:
            throw http_error("invalid request-line?", HTTP::STATUS_BAD_REQUEST);
    }
    DXOUT("* parsed reqline *");
    start_of_current_header = mid;
}

void    RequestHTTP::parse_header_lines(const byte_string& bytebuffer, ssize_t from, ssize_t len) {
    DXOUT("\"\"\"" << std::endl
        << byte_string(bytebuffer.begin() + from, bytebuffer.begin() + from + len)
        << std::endl
        << "\"\"\"");
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
            DXOUT("found obs-fold: " << res);
            ps.found_obs_fold = true;
            movement = res.second;
        }
        // DSOUT() << "sval: \"" << sval << "\"" << std::endl;
    }

    DXOUT("* splitted a header *"
        << std::endl
        << "Key: " << key
        << std::endl
        << "Val: " << sval);
    header_holder.add_item(key, sval);
}

void    RequestHTTP::extract_control_headers() {
    // 取得したヘッダから制御用の情報を抽出する.
    // TODO: ここで何を抽出すべきか洗い出す

    cp.determine_host(header_holder);
    cp.determine_content_type(header_holder);
    cp.determine_transfer_encoding(header_holder);
    cp.determine_body_size(header_holder);
    cp.determine_connection(header_holder);
    cp.determine_te(header_holder);
    cp.determine_upgrade(header_holder);
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
void    RequestHTTP::ControlParams::determine_host(const HeaderHTTPHolder& holder) {
    // https://triple-underscore.github.io/RFC7230-ja.html#header.host
    const HeaderHTTPHolder::value_list_type *hosts = holder.get_vals(HeaderHTTP::host);
    if (!hosts || hosts->size() == 0) {
        // HTTP/1.1 なのに host がない場合, BadRequest を出す
        if (http_version == HTTP::V_1_1) {
            throw http_error("no host for HTTP/1.1", HTTP::STATUS_BAD_REQUEST);
        }
        return;
    }
    if (hosts->size() > 1) {
        // Hostが複数ある場合, BadRequest を出す
        throw http_error("multiple hosts", HTTP::STATUS_BAD_REQUEST);
    }
    // Hostの値のバリデーション (と, 必要ならBadRequest)
    const HTTP::light_string lhost(hosts->front());
    if (!HTTP::Validator::is_valid_header_host(lhost)) {
        throw http_error("host is not valid", HTTP::STATUS_BAD_REQUEST);
    }
    header_host.value = lhost.get_base();
    // この時点で lhost は Host: として妥当
    // -> 1文字目が [ かどうかで ipv6(vfuture) かどうかを判別する.
    if (lhost[0] == '[') {
        // ipv6 or ipvfuture
        HTTP::light_string::size_type i = lhost.find_last_of("]");
        header_host.host = lhost.substr(0, i + 1).str();
        if (i + 1 < lhost.size()) {
            header_host.port = lhost.substr(i + 2).str();
        }
    } else {
        // ipv4 or reg-name
        HTTP::light_string::size_type i = lhost.find_last_of(":");
        header_host.host = lhost.substr(0, i).str();
        if (i != HTTP::light_string::npos) {
            header_host.port = lhost.substr(i + 1).str();
        }
    }
    DXOUT("host: \"" << header_host.host << "\"");
    DXOUT("port: \"" << header_host.port << "\"");
}

void RequestHTTP::ControlParams::determine_transfer_encoding(const HeaderHTTPHolder& holder) {
    // https://httpwg.org/specs/rfc7230.html#header.transfer-encoding
    // Transfer-Encoding  = 1#transfer-coding
    // transfer-coding    = "chunked"
    //                    / "compress"
    //                    / "deflate"
    //                    / "gzip"
    //                    / transfer-extension
    // transfer-extension = token *( OWS ";" OWS transfer-parameter )
    // transfer-parameter = token BWS "=" BWS ( token / quoted-string )

    const HeaderHTTPHolder::value_list_type *tes = holder.get_vals(HeaderHTTP::transfer_encoding);
    if (!tes) { return; }
    for (HeaderHTTPHolder::value_list_type::const_iterator it = tes->begin(); it != tes->end(); ++it) {
        light_string    val_lstr = light_string(*it);
        for (;;) {
            DXOUT("val_lstr: \"" << val_lstr << "\"");
            val_lstr = val_lstr.substr_after(HTTP::CharFilter::sp);
            if (val_lstr.size() == 0) {
                DXOUT("away; sp only.");
                break;
            }
            light_string tc_lstr = val_lstr.substr_while(HTTP::CharFilter::tchar);
            if (tc_lstr.size() == 0) {
                DXOUT("away; no value.");
                break;
            }

            // 本体
            DXOUT("tc_lstr: \"" << tc_lstr << "\"");
            HTTP::Term::TransferCoding  tc;
            tc.coding = tc_lstr.str();
            transfer_encoding.transfer_codings.push_back(tc);
            
            // 後続
            val_lstr = val_lstr.substr_after(HTTP::CharFilter::sp, tc_lstr.size());
            if (val_lstr.size() == 0) {
                DXOUT("away");
                break;
            }
            // cat(sp_end) が "," か ";" かによって分岐
            DXOUT("val_lstr[0]: " << val_lstr[0]);
            if (val_lstr[0] == ',') {
                // 次の要素
                val_lstr = val_lstr.substr(1);
            } else if (val_lstr[0] == ';') {
                // parameterがはじまる
                val_lstr = decompose_semicoron_separated_kvlist(val_lstr, transfer_encoding.transfer_codings.back());
            } else {
                // 不正な要素
                DXOUT("[KO] unexpected state: \"" << val_lstr.substr(0) << "\"");
                break;
            }
        }
    }
    transfer_encoding.currently_chunked =
        !transfer_encoding.empty() &&
        transfer_encoding.current_coding().coding == "chunked";
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
        content_type.value = HTTP::CH::ContentType::default_value;
        return;
    }
    const light_string lct(*ct);
    light_string::size_type type_end = lct.find_first_not_of(HTTP::CharFilter::tchar);
    if (type_end == light_string::npos) {
        DXOUT("[KO] no /: \"" << lct << "\"");
        return;
    }
    if (lct[type_end] != '/') {
        DXOUT("[KO] not separated by /: \"" << lct << "\"");
        return;
    }
    light_string    type_str(lct, 0, type_end);
    light_string::size_type subtype_end = lct.find_first_not_of(HTTP::CharFilter::tchar, type_end + 1);
    light_string    subtype_str(lct, type_end + 1, subtype_end);
    if (subtype_str.size() == 0) {
        DXOUT("[KO] no subtype after /: \"" << lct << "\"");
        return;
    }

    if (subtype_end == light_string::npos) {
        return;
    }
    content_type.value = lct.substr(0, subtype_end).str();
    DXOUT("content_type.value: " << content_type.value);

    // media-type     = type "/" subtype *( OWS ";" OWS parameter )
    // light_string    parameters_str(lct, subtype_end);
    // light_string    continuation = decompose_semicoron_separated_kvlist(parameters_str, content_type);
    // DXOUT("parameter: " << content_type.parameters.size());
    // DXOUT("continuation: \"" << continuation << "\"");
}

void    RequestHTTP::ControlParams::determine_connection(const HeaderHTTPHolder& holder) {
    // Connection        = 1#connection-option
    // connection-option = token
    const HeaderHTTPHolder::value_list_type *cons = holder.get_vals(HeaderHTTP::connection);
    if (!cons) { return; }
    if (cons->size() == 0) {
        DXOUT("[KO] list exists, but it's empty.");
        return;
    }
    for (HeaderHTTPHolder::value_list_type::const_iterator it = cons->begin(); it != cons->end(); ++it) {
        light_string    val_lstr = light_string(*it);
        for (;;) {
            DXOUT("val_lstr: \"" << val_lstr << "\"");
            val_lstr = val_lstr.substr_after(HTTP::CharFilter::sp);
            if (val_lstr.size() == 0) {
                DXOUT("away; sp only.");
                break;
            }
            light_string target_lstr = val_lstr.substr_while(HTTP::CharFilter::tchar);
            if (target_lstr.size() == 0) {
                DXOUT("away; no value.");
                break;
            }

            // 本体
            byte_string target_str = HTTP::Utils::downcase(target_lstr.str());
            DXOUT("target_str: \"" << target_str << "\"");
            connection.connection_options.push_back(target_str);
            if (target_str == "close") {
                connection.close_ = true;
            } else if (target_str == "keep-alive") {
                connection.keep_alive_ = true;
            }

            // 後続
            val_lstr = val_lstr.substr_after(HTTP::CharFilter::sp, target_lstr.size());
            if (val_lstr.size() == 0) {
                DXOUT("away");
                break;
            }
            // cat(sp_end) が "," か ";" かによって分岐
            DXOUT("val_lstr[0]: " << val_lstr[0]);
            if (val_lstr[0] == ',') {
                // 次の要素
                val_lstr = val_lstr.substr(1);
            } else {
                // 不正な要素
                DXOUT("[KO] unexpected state: \"" << val_lstr.substr(0) << "\"");
                break;
            }
        }
    }
    DXOUT("close: " << connection.will_close() << ", keep-alive: " << connection.will_keep_alive());
}

void RequestHTTP::ControlParams::determine_te(const HeaderHTTPHolder& holder) {
    // https://triple-underscore.github.io/RFC7230-ja.html#header.te
    // TE        = #t-codings
    // t-codings = "trailers" / ( transfer-coding [ t-ranking ] )
    // t-ranking = OWS ";" OWS "q=" rank
    // rank      = ( "0" [ "." 0*3DIGIT ] )
    //           / ( "1" [ "." 0*3("0") ] )
    // transfer-coding    = "chunked"
    //                    / "compress"
    //                    / "deflate"
    //                    / "gzip"
    //                    / transfer-extension
    // transfer-extension = token *( OWS ";" OWS transfer-parameter )
    // transfer-parameter = token BWS "=" BWS ( token / quoted-string )

    const HeaderHTTPHolder::value_list_type *tes = holder.get_vals(HeaderHTTP::te);
    if (!tes) { return; }
    for (HeaderHTTPHolder::value_list_type::const_iterator it = tes->begin(); it != tes->end(); ++it) {
        light_string    val_lstr = light_string(*it);
        for (;;) {
            DXOUT("val_lstr: \"" << val_lstr << "\"");
            val_lstr = val_lstr.substr_after(HTTP::CharFilter::sp);
            if (val_lstr.size() == 0) {
                DXOUT("away; sp only.");
                break;
            }
            light_string tc_lstr = val_lstr.substr_while(HTTP::CharFilter::tchar);
            if (tc_lstr.size() == 0) {
                DXOUT("away; no value.");
                break;
            }

            // 本体
            DXOUT("tc_lstr: \"" << tc_lstr << "\"");
            HTTP::Term::TransferCoding  tc = HTTP::Term::TransferCoding::init();
            tc.coding = tc_lstr.str();
            te.transfer_codings.push_back(tc);
            
            // 後続
            val_lstr = val_lstr.substr_after(HTTP::CharFilter::sp, tc_lstr.size());
            if (val_lstr.size() == 0) {
                DXOUT("away");
                break;
            }

            DXOUT("val_lstr[0]: " << val_lstr[0]);
            HTTP::Term::TransferCoding& last_coding = te.transfer_codings.back();
            if (val_lstr[0] == ';') {
                // parameterがはじまる
                val_lstr = decompose_semicoron_separated_kvlist(val_lstr, last_coding);
            }

            // rankがあるなら, それはセミコロン分割リストの一部として解釈されるはず -> q 値をチェック.
            if (!te.transfer_codings.empty()) {
                HTTP::IDictHolder::parameter_dict::iterator qit
                    = last_coding.parameters.find("q");
                if (qit != last_coding.parameters.end()) {
                    DXOUT("rank: \"" << qit->second << "\"");
                    if (HTTP::Validator::is_valid_rank(qit->second)) {
                        DXOUT("rank is valid: " << qit->second);
                        DXOUT("q: " << last_coding.quality_int);
                        last_coding.quality_int = ParserHelper::quality_to_u(qit->second);
                        DXOUT("q -> " << last_coding.quality_int);
                    } else {
                        DXOUT("rank is INVALID: " << qit->second);
                    }
                }
            }

            if (val_lstr[0] == ',') {
                // 次の要素
                val_lstr = val_lstr.substr(1);
            } else {
                // 不正な要素
                DXOUT("[KO] unexpected state: \"" << val_lstr.substr(0) << "\"");
                break;
            }
        }
    }
}

void RequestHTTP::ControlParams::determine_upgrade(const HeaderHTTPHolder& holder) {
    // Upgrade          = 1#protocol
    // protocol         = protocol-name ["/" protocol-version]
    // protocol-name    = token
    // protocol-version = token
    const HeaderHTTPHolder::value_list_type *elems = holder.get_vals(HeaderHTTP::upgrade);
    if (!elems) { return; }
    for (HeaderHTTPHolder::value_list_type::const_iterator it = elems->begin(); it != elems->end(); ++it) {
        light_string    val_lstr = light_string(*it);
        for (;val_lstr.size() > 0;) {
            val_lstr = val_lstr.substr_after(HTTP::CharFilter::sp);
            light_string name = val_lstr.substr_while(HTTP::CharFilter::tchar);
            if (name.size() == 0) {
                // 値なし
                DXOUT("[KO] no name");
                break;
            }
            HTTP::Term::Protocol    protocol;
            protocol.name = name;
            val_lstr = val_lstr.substr(name.size());
            if (val_lstr.size() > 0 && val_lstr[0] == '/') {
                // versionがある
                light_string version = val_lstr.substr_while(HTTP::CharFilter::tchar, 1);
                if (version.size() == 0) {
                    // 値なし
                    DXOUT("[KO] no version after slash");
                } else {
                    protocol.version = version;
                }
                val_lstr = val_lstr.substr(1 + version.size());
            }
            DXOUT("protocol: " << protocol.name.qstr() << " - " << protocol.version.qstr());
            upgrade.protocols.push_back(protocol);
            val_lstr = val_lstr.substr_after(HTTP::CharFilter::sp);
            if (val_lstr.size() == 0 || val_lstr[0] != ',') {
                DXOUT("away");
                break;
            }
            val_lstr = val_lstr.substr(1);
        }
    }
}

RequestHTTP::light_string
    RequestHTTP::ControlParams::decompose_semicoron_separated_kvlist(
        const light_string& kvlist_str,
        HTTP::IDictHolder& holder
    ) {
    light_string list_str = kvlist_str;
    // *( OWS ";" OWS parameter )
    for (;;) {
        light_string  params_str = list_str;
        DXOUT("params_str: \"" << params_str << "\"");
        params_str = params_str.substr_after(HTTP::CharFilter::sp);
        if (params_str.size() == 0) {
            DXOUT("away, there's only sp.");
            return params_str;
        }
        if (params_str[0] != ';') {
            DXOUT("away");
            return params_str;
        }
        params_str = params_str.substr_after(HTTP::CharFilter::sp, 1);
        // DXOUT("param_sep2: " << sep_pos);
        if (params_str.size() == 0) {
            DXOUT("[KO] there's only sp after ';'");
            return params_str;
        }
        // ここからparameterを取得
        // token
        const light_string key_lstr = params_str.substr_while(HTTP::CharFilter::tchar);
        if (key_lstr.size() == params_str.size() || params_str[key_lstr.size()] != '=') {
            DXOUT("[KO] no equal");
            return params_str.substr(params_str.size());
        }
        // [引数名]
        // 引数名はcase-insensitive
        // https://wiki.suikawiki.org/n/Content-Type#anchor-11
        // > 引数名は、大文字・小文字の区別なしで定義されています。
        params_str = params_str.substr(key_lstr.size() + 1);

        const light_string  value_str = params_str;
        byte_string         key_str = ParserHelper::normalize_header_key(key_lstr);
        DXOUT("key: \"" << key_str << "\"");
        {
            //                  [key]            [value]
            // parameter      = token "=" ( token / quoted-string )
            if (HTTP::CharFilter::dquote.includes(value_str[0])) {
                // quoted-string  = DQUOTE *( qdtext / quoted-pair ) DQUOTE
                // qdtext         = HTAB / SP / %x21 / %x23-5B / %x5D-7E / obs-text
                //                ;               !     #-[        ]-~
                //                ; HTAB + 表示可能文字, ただし "(ダブルクオート) と \(バッスラ) を除く
                // obs-text       = %x80-FF ; extended ASCII
                // quoted-pair    = "\" ( HTAB / SP / VCHAR / obs-text )
                light_string::size_type i = 1;
                bool    quoted = false;
                for(;i < value_str.size(); ++i) {
                    const HTTP::byte_type c = value_str[i];
                    if (!quoted && HTTP::CharFilter::dquote.includes(c)) {
                        // クオートされてないダブルクオート
                        // -> ここで終わり
                        break;
                    }
                    if (quoted && !HTTP::CharFilter::quoted_right.includes(c)) {
                        // クオートの右がquoted_rightではない
                        // -> 不適格
                        DXOUT("[KO] not suitable for quote");
                        return params_str.substr(params_str.size());
                    }
                    if (!quoted && HTTP::CharFilter::bslash.includes(c)) {
                        // クオートされてないバックスラッシュ
                        // -> クオートフラグ立てる
                        quoted = true;
                    } else {
                        quoted = false;
                    }
                }
                // DXOUT("quoted-string: \"" << light_string(value_str, 0, i + 1) << "\"");
                list_str = value_str.substr(i + 1);
                DXOUT("value: \"" << value_str.substr(0, i + 1) << "\"");
                holder.store_list_item(key_str, value_str.substr(0, i + 1));
            } else {
                // token          = 1*tchar
                const light_string::size_type value_end = value_str.find_first_not_of(HTTP::CharFilter::tchar);
                if (value_end == 0) {
                    DXOUT("[KO] empty value");
                    return params_str.substr(params_str.size());
                }
                light_string just_value_str(value_str, 0, value_end);
                DXOUT("value: \"" << just_value_str << "\"");
                list_str = value_str.substr(value_end);
                holder.store_list_item(key_str, just_value_str);
            }
        }
    }
    return list_str;
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
