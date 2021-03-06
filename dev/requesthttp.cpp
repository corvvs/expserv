#include "requesthttp.hpp"

HTTP::t_method discriminate_request_method(const HTTP::light_string &str) {
    if (str == "GET") {
        return HTTP::METHOD_GET;
    }
    if (str == "POST") {
        return HTTP::METHOD_POST;
    }
    if (str == "DELETE") {
        return HTTP::METHOD_DELETE;
    }
    throw http_error("unsupported method", HTTP::STATUS_METHOD_NOT_ALLOWED);
}

HTTP::t_version discriminate_request_version(const HTTP::light_string &str) {
    if (str == HTTP::version_str(HTTP::V_1_0)) {
        return HTTP::V_1_0;
    }
    if (str == HTTP::version_str(HTTP::V_1_1)) {
        return HTTP::V_1_1;
    }
    throw http_error("unsupported version", HTTP::STATUS_VERSION_NOT_SUPPORTED);
}

RequestHTTP::ParserStatus::ParserStatus() : found_obs_fold(false) {}

RequestHTTP::RequestHTTP() : mid(0), cp() {
    this->ps.parse_progress = PP_REQLINE_START;
    this->cp.http_method    = HTTP::METHOD_UNKNOWN;
    this->cp.http_version   = HTTP::V_UNKNOWN;
    bytebuffer.reserve(MAX_REQLINE_END);
}

RequestHTTP::~RequestHTTP() {}

void RequestHTTP::feed_bytestring(char *bytes, size_t feed_len) {
    bytebuffer.insert(bytebuffer.end(), bytes, bytes + feed_len);
    bool is_disconnected = feed_len == 0;
    size_t len           = 1;
    t_parse_progress flow;
    do {
        len = bytebuffer.size() - this->mid;
        switch (this->ps.parse_progress) {
            case PP_REQLINE_START: {
                flow = reach_reqline_start(len, is_disconnected);
                if (flow == PP_UNREACHED) {
                    return;
                }
                this->ps.parse_progress = flow;
                continue;
            }

            case PP_REQLINE_END: {
                flow = reach_reqline_end(len, is_disconnected);
                if (flow == PP_UNREACHED) {
                    return;
                }
                this->ps.parse_progress = flow;
                continue;
            }

            case PP_HEADER_SECTION_END: {
                flow = reach_headers_end(len, is_disconnected);
                if (flow == PP_UNREACHED) {
                    return;
                }
                this->ps.parse_progress = flow;
                continue;
            }

            case PP_BODY: {
                flow = reach_fixed_body_end(len, is_disconnected);
                if (flow == PP_UNREACHED) {
                    return;
                }
                this->ps.parse_progress = flow;
                continue;
            }

            case PP_CHUNK_SIZE_LINE_END: {
                flow = reach_chunked_size_line(len, is_disconnected);
                if (flow == PP_UNREACHED) {
                    return;
                }
                this->ps.parse_progress = flow;
                continue;
            }

            case PP_CHUNK_DATA_END: {
                flow = reach_chunked_data_end(len, is_disconnected);
                if (flow == PP_UNREACHED) {
                    return;
                }
                this->ps.parse_progress = flow;
                continue;
            }

            case PP_CHUNK_DATA_CRLF: {
                flow = reach_chunked_data_termination(len, is_disconnected);
                if (flow == PP_UNREACHED) {
                    return;
                }
                this->ps.parse_progress = flow;
                continue;
            }

            case PP_TRAILER_FIELD_END: {
                flow = reach_chunked_trailer_end(len, is_disconnected);
                if (flow == PP_UNREACHED) {
                    return;
                }
                this->ps.parse_progress = flow;
                continue;
            }

            case PP_OVER: {
                return;
            }

            default:
                throw http_error("not implemented yet", HTTP::STATUS_NOT_IMPLEMENTED);
        }
    } while (len > 0);
}

RequestHTTP::t_parse_progress RequestHTTP::reach_reqline_start(size_t len, bool is_disconnected) {
    (void)is_disconnected;
    DXOUT("* determining start_of_reqline... *");
    size_t s_o_s = ParserHelper::ignore_crlf(bytebuffer, this->mid, len);
    this->mid += s_o_s;
    if (s_o_s == len) {
        return PP_UNREACHED;
    }
    // ???????????????????????????????????????
    this->ps.start_of_reqline = this->mid;
    return PP_REQLINE_END;
}

RequestHTTP::t_parse_progress RequestHTTP::reach_reqline_end(size_t len, bool is_disconnected) {
    (void)is_disconnected;
    if (!seek_reqline_end(len)) {
        return PP_UNREACHED;
    }
    light_string raw_req_line(bytebuffer, this->ps.start_of_reqline, this->ps.end_of_reqline);
    // -> [start_of_reqline, end_of_reqline) ??? ??????????????????????????????.
    parse_reqline(raw_req_line);
    return PP_HEADER_SECTION_END;
}

RequestHTTP::t_parse_progress RequestHTTP::reach_headers_end(size_t len, bool is_disconnected) {
    // ???????????????????????????????????????
    // `crlf_in_header` ??????????????????????????????CRLF?????????????????????????????????.
    // mid??????????????????CRLF???????????????:
    // - ???????????????
    //   - first == crlf_in_header.second ?????????
    //     -> ?????????. ??????CRLF?????????????????????????????????.
    //   - ???????????????
    //     -> ?????????. ??????CRLF??? crlf_in_header ???????????????.
    // - ????????????????????????
    //   -> ????????????????????????
    (void)is_disconnected;
    IndexRange res = ParserHelper::find_crlf(bytebuffer, this->mid, len);
    this->mid      = res.second;
    if (res.is_invalid()) {
        return PP_UNREACHED;
    }
    if (this->ps.crlf_in_header.second != res.first) {
        // ?????????: CRLF???CRLF????????????????????????
        this->ps.crlf_in_header = res;
        return PP_HEADER_SECTION_END;
    }
    // ?????????: ???????????????????????????????????????
    analyze_headers(res);
    if (this->cp.is_body_chunked) {
        return PP_CHUNK_SIZE_LINE_END;
    } else {
        return PP_BODY;
    }
}

RequestHTTP::t_parse_progress RequestHTTP::reach_fixed_body_end(size_t len, bool is_disconnected) {
    // TODO: ????????????
    // - ???????????????????????????
    //   -> ???????????????body?????????
    // - content-length???????????????
    //   -> ??????
    // - ???????????????????????? content-length ????????????????????????
    //   -> ??????
    // - (else) ???????????????????????? content-length ????????????
    //   -> ????????????????????? = this->mid - start_of_body ??? content-length ?????????????????????
    (void)is_disconnected;
    this->mid += len;
    if (parsed_body_size() < this->cp.body_size) {
        return PP_UNREACHED;
    }
    this->ps.end_of_body = this->cp.body_size + this->ps.start_of_body;
    this->mid            = this->ps.end_of_body;
    return PP_OVER;
}

RequestHTTP::t_parse_progress RequestHTTP::reach_chunked_size_line(size_t len, bool is_disconnected) {
    (void)is_disconnected;
    IndexRange res = ParserHelper::find_crlf(bytebuffer, this->mid, len);
    this->mid      = res.second;
    if (res.is_invalid()) {
        return PP_UNREACHED;
    }
    // [start_of_current_chunk, res.first) ????????????????????????.
    light_string chunk_size_line(bytebuffer, this->ps.start_of_current_chunk, res.first);
    QVOUT(chunk_size_line);
    parse_chunk_size_line(chunk_size_line);
    // TODO: ?????????????????????
    VOUT(this->ps.current_chunk.chunk_size);
    if (this->ps.current_chunk.chunk_size == 0) {
        // ???????????????????????? PP_TRAILER_FIELD_END ????????????
        this->ps.crlf_in_header         = res;
        this->ps.start_of_trailer_field = res.second;
        VOUT(chunked_body.size());
        VOUT(chunked_body.body());
        VOUT(this->ps.crlf_in_header.first);
        VOUT(this->ps.crlf_in_header.second);
        VOUT(this->mid);
        return PP_TRAILER_FIELD_END;
    } else {
        this->ps.start_of_current_chunk_data = this->mid;
        return PP_CHUNK_DATA_END;
    }
}

RequestHTTP::t_parse_progress RequestHTTP::reach_chunked_data_end(size_t len, bool is_disconnected) {
    (void)is_disconnected;
    const byte_string::size_type received_data_size = this->mid - this->ps.start_of_current_chunk_data;
    const byte_string::size_type data_end           = this->ps.current_chunk.chunk_size - received_data_size;
    if (data_end > len) {
        this->mid += len;
        return PP_UNREACHED;
    }
    this->mid += data_end;
    this->ps.current_chunk.chunk_str = light_string(bytebuffer, this->ps.start_of_current_chunk, this->mid);
    this->ps.current_chunk.data_str  = light_string(bytebuffer, this->ps.start_of_current_chunk_data, this->mid);
    QVOUT(this->ps.current_chunk.chunk_str);
    QVOUT(this->ps.current_chunk.data_str);
    return PP_CHUNK_DATA_CRLF;
}

RequestHTTP::t_parse_progress RequestHTTP::reach_chunked_data_termination(size_t len, bool is_disconnected) {
    VOUT(this->mid);
    IndexRange nl = ParserHelper::find_leading_crlf(bytebuffer, this->mid, len, is_disconnected);
    if (nl.is_invalid()) {
        throw http_error("invalid chunk-data end?", HTTP::STATUS_BAD_REQUEST);
    }
    if (nl.length() == 0) {
        return PP_UNREACHED;
    }
    this->ps.current_chunk.data_str = light_string(bytebuffer, this->ps.start_of_current_chunk_data, this->mid);
    this->mid                       = nl.second;
    this->ps.start_of_current_chunk = this->mid;
    chunked_body.add_chunk(this->ps.current_chunk);
    VOUT(this->mid);
    return PP_CHUNK_SIZE_LINE_END;
}

RequestHTTP::t_parse_progress RequestHTTP::reach_chunked_trailer_end(size_t len, bool is_disconnected) {
    // ?????????????????????????????????????????????????????????
    // `crlf_in_header` ??????????????????????????????CRLF?????????????????????????????????.
    // mid??????????????????CRLF???????????????:
    // - ???????????????
    //   - first == crlf_in_header.second ?????????
    //     -> ?????????. ??????CRLF?????????????????????????????????.
    //   - ???????????????
    //     -> ?????????. ??????CRLF??? crlf_in_header ???????????????.
    // - ????????????????????????
    //   -> ????????????????????????
    (void)is_disconnected;
    IndexRange res = ParserHelper::find_crlf(bytebuffer, this->mid, len);
    this->mid      = res.second;
    if (res.is_invalid()) {
        return PP_UNREACHED;
    }
    if (this->ps.crlf_in_header.second != res.first) {
        // ?????????
        this->ps.crlf_in_header = res;
        return PP_TRAILER_FIELD_END;
    }
    // ?????????
    this->ps.end_of_trailer_field = res.first;
    const light_string trailer_field_lines(bytebuffer, this->ps.start_of_trailer_field, this->ps.end_of_trailer_field);
    parse_header_lines(trailer_field_lines, NULL);
    return PP_OVER;
}

void RequestHTTP::analyze_headers(IndexRange res) {
    this->ps.end_of_header = res.first;
    this->ps.start_of_body = res.second;
    // -> [start_of_header, end_of_header) ???????????????
    const light_string header_lines(bytebuffer, this->ps.start_of_header, this->ps.end_of_header);
    parse_header_lines(header_lines, &this->header_holder);
    extract_control_headers();
    VOUT(this->cp.is_body_chunked);
    if (this->cp.is_body_chunked) {
        this->ps.start_of_current_chunk = this->ps.start_of_body;
    }
}

bool RequestHTTP::seek_reqline_end(size_t len) {
    // DSOUT() << "* determining end_of_reqline... *" << std::endl;
    IndexRange res = ParserHelper::find_crlf(bytebuffer, this->mid, len);
    this->mid      = res.second;
    if (res.is_invalid()) {
        return false;
    }
    // CRLF??????????????????
    this->ps.end_of_reqline  = res.first;
    this->ps.start_of_header = this->mid;
    // -> end_of_reqline ???8192???????????????????????????????????????
    if (MAX_REQLINE_END <= this->ps.end_of_reqline) {
        throw http_error("Invalid Response: request line is too long", HTTP::STATUS_URI_TOO_LONG);
    }
    this->ps.crlf_in_header = IndexRange(this->ps.start_of_header, this->ps.start_of_header);
    return true;
}

void RequestHTTP::parse_reqline(const light_string &raw_req_line) {
    std::vector<light_string> splitted = ParserHelper::split_by_sp(raw_req_line);

    switch (splitted.size()) {
        case 2:
        case 3: {
            // HTTP/0.9?
            // HTTP/1.*?

            this->cp.http_method = discriminate_request_method(splitted[0]);
            DXOUT(splitted[0] << " -> http_method: " << this->cp.http_method);
            this->cp.request_path = splitted[1];
            DXOUT("request_path: " << this->cp.request_path);
            if (splitted.size() == 3) {
                this->cp.http_version = discriminate_request_version(splitted[2]);
            } else {
                this->cp.http_version = HTTP::V_0_9;
            }
            DXOUT(splitted[2] << " -> http_version: " << this->cp.http_version);
            break;
        }
        default:
            throw http_error("invalid request-line?", HTTP::STATUS_BAD_REQUEST);
    }
    DXOUT("* parsed reqline *");
}

void RequestHTTP::parse_header_lines(const light_string &lines, HeaderHTTPHolder *holder) const {
    light_string rest(lines);
    DXOUT(std::endl << "==============" << std::endl << rest << std::endl << "==============");
    while (true) {
        QVOUT(rest);
        const IndexRange res = ParserHelper::find_crlf_header_value(rest);
        if (res.is_invalid()) {
            break;
        }
        const light_string header_line = rest.substr(0, res.first);
        QVOUT(header_line);
        if (header_line.length() > 0) {
            // header_line ????????????????????????
            // -> ??????????????????????????????????????????
            parse_header_line(header_line, holder);
        }
        rest = rest.substr(res.second);
    }
}

void RequestHTTP::parse_header_line(const light_string &line, HeaderHTTPHolder *holder) const {

    const light_string key = line.substr_before(ParserHelper::HEADER_KV_SPLITTER);
    QVOUT(line);
    QVOUT(key);
    if (key.length() == line.length()) {
        // [!] Apache ??? : ?????????????????????????????????????????????????????????????????????????????? 400 ???????????????????????? nginx
        // ???????????????????????????????????????
        throw http_error("no coron in a header line", HTTP::STATUS_BAD_REQUEST);
    }
    // ":"???????????? -> ":"??????????????????????????????????????????
    if (key.length() == 0) {
        throw http_error("header key is empty", HTTP::STATUS_BAD_REQUEST);
    }
    light_string val = line.substr(key.length() + 1);
    // [!] ????????? : ??????????????????????????????????????????????????? ?????????????????????????????? 400 ?????????????????????????????????????????????????????????
    // ????????????????????????????????????????????????????????????????????????????????????
    light_string::size_type key_tail = key.find_last_not_of(ParserHelper::OWS);
    if (key_tail + 1 != key.length()) {
        throw http_error("trailing space on header key", HTTP::STATUS_BAD_REQUEST);
    }
    // [!] ?????????????????? OWS ???????????????????????????????????? ????????????????????????????????????
    val = val.trim(ParserHelper::OWS);
    // holder ??????????????? holder ?????????. ?????????????????? holder ????????????.
    if (holder != NULL) {
        holder->add_item(key, val);
    } else {
        DXOUT("no holder");
        QVOUT(key);
        QVOUT(val);
    }
}

void RequestHTTP::parse_chunk_size_line(const light_string &line) {
    light_string size_str = line.substr_while(HTTP::CharFilter::hexdig);
    if (size_str.size() == 0) {
        throw http_error("no size for chunk", HTTP::STATUS_BAD_REQUEST);
    }
    // chunk-size ?????????
    // chunk-size   = 1*HEXDIG
    this->ps.current_chunk.size_str = size_str;
    QVOUT(this->ps.current_chunk.size_str);
    std::pair<bool, unsigned int> x = ParserHelper::xtou(size_str);
    if (!x.first) {
        throw http_error("invalid size for chunk", HTTP::STATUS_BAD_REQUEST);
    }
    this->ps.current_chunk.chunk_size = x.second;
    ;
    VOUT(this->ps.current_chunk.chunk_size);
    // chunk-ext ?????????(?????????????????????)
    // chunk-ext    = *( BWS ";" BWS chunk-ext-name [ BWS "=" BWS chunk-ext-val ] )
    // chunk-ext-name = token
    // chunk-ext-val  = token / quoted-string
    light_string rest = line.substr(size_str.size());
    for (;;) {
        QVOUT(rest);
        rest = rest.substr_after(HTTP::CharFilter::bad_sp);
        if (rest.size() == 0) {
            DXOUT("away");
            break;
        }
        if (rest[0] != ';') {
            DXOUT("[KO] bad separator");
            break;
        }
        rest = rest.substr_after(HTTP::CharFilter::bad_sp, 1);
        QVOUT(rest);
        const light_string chunk_ext_name = rest.substr_while(HTTP::CharFilter::tchar);
        QVOUT(chunk_ext_name);
        if (chunk_ext_name.size() == 0) {
            DXOUT("[KO] no chunk_ext_name");
            break;
        }
        rest = rest.substr(chunk_ext_name.size());
        rest = rest.substr_after(HTTP::CharFilter::bad_sp);
        QVOUT(rest);
        if (rest.size() > 0 && rest[0] == '=') {
            // chunk-ext-val ?????????
            rest                             = rest.substr(1);
            const light_string chunk_ext_val = ParserHelper::extract_quoted_or_token(rest);
            QVOUT(chunk_ext_val);
            if (chunk_ext_val.size() == 0) {
                DXOUT("[KO] zero-width chunk_ext_val");
                break;
            }
            rest = rest.substr(chunk_ext_val.size());
        }
    }
}

void RequestHTTP::extract_control_headers() {
    // ????????????????????????????????????????????????????????????.
    // TODO: ?????????????????????????????????????????????

    this->cp.determine_host(header_holder);
    this->cp.determine_content_type(header_holder);
    this->cp.determine_transfer_encoding(header_holder);
    this->cp.determine_body_size(header_holder);
    this->cp.determine_connection(header_holder);
    this->cp.determine_te(header_holder);
    this->cp.determine_upgrade(header_holder);
    this->cp.determine_via(header_holder);
}

void RequestHTTP::ControlParams::determine_body_size(const HeaderHTTPHolder &holder) {
    // https://datatracker.ietf.org/doc/html/rfc7230#section-3.3.3

    if (transfer_encoding.currently_chunked) {
        // ??????????????????????????????Transfer-Encoding????????????, ?????????????????????coding???chunked???????????????
        // -> chunk??????????????????????????????
        DXOUT("by chunk");
        is_body_chunked = true;
        return;
    }

    if (transfer_encoding.empty()) {
        // Transfer-Encoding?????????, Content-Length?????????????????????
        // -> ???????????????????????????
        const byte_string *cl = holder.get_val(HeaderHTTP::content_length);
        // Transfer-Encoding?????????, Content-Length????????????????????????
        // -> Content-Length ????????????????????????????????????.
        if (cl) {
            body_size       = ParserHelper::stou(*cl);
            is_body_chunked = false;
            // content-length ??????????????????????????????, ????????????????????????
            DXOUT("body_size = " << body_size);
            return;
        }
    }

    // ?????????????????????0.
    body_size       = 0;
    is_body_chunked = false;
    DXOUT("body_size is zero.");
}
void RequestHTTP::ControlParams::determine_host(const HeaderHTTPHolder &holder) {
    // https://triple-underscore.github.io/RFC7230-ja.html#header.host
    const HeaderHTTPHolder::value_list_type *hosts = holder.get_vals(HeaderHTTP::host);
    if (!hosts || hosts->size() == 0) {
        // HTTP/1.1 ????????? host ???????????????, BadRequest ?????????
        if (http_version == HTTP::V_1_1) {
            throw http_error("no host for HTTP/1.1", HTTP::STATUS_BAD_REQUEST);
        }
        return;
    }
    if (hosts->size() > 1) {
        // Host?????????????????????, BadRequest ?????????
        throw http_error("multiple hosts", HTTP::STATUS_BAD_REQUEST);
    }
    // Host?????????????????????????????? (???, ????????????BadRequest)
    const HTTP::light_string lhost(hosts->front());
    if (!HTTP::Validator::is_valid_header_host(lhost)) {
        throw http_error("host is not valid", HTTP::STATUS_BAD_REQUEST);
    }
    pack_host(header_host, lhost);
}

void RequestHTTP::ControlParams::determine_transfer_encoding(const HeaderHTTPHolder &holder) {
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
    if (!tes) {
        return;
    }
    int i = 0;
    for (HeaderHTTPHolder::value_list_type::const_iterator it = tes->begin(); it != tes->end(); ++it) {
        ++i;
        VOUT(i);
        light_string val_lstr = light_string(*it);
        for (;;) {
            QVOUT(val_lstr);
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

            // ??????
            QVOUT(tc_lstr);
            HTTP::Term::TransferCoding tc;
            tc.coding = tc_lstr.str();
            QVOUT(tc.coding);
            transfer_encoding.transfer_codings.push_back(tc);
            QVOUT(transfer_encoding.transfer_codings.back().coding);

            // ??????
            val_lstr = val_lstr.substr_after(HTTP::CharFilter::sp, tc_lstr.size());
            if (val_lstr.size() == 0) {
                DXOUT("away");
                break;
            }
            // cat(sp_end) ??? "," ??? ";" ?????????????????????
            QVOUT(val_lstr);
            if (val_lstr[0] == ';') {
                // parameter???????????????
                val_lstr = decompose_semicoron_separated_kvlist(val_lstr, transfer_encoding.transfer_codings.back());
            }
            QVOUT(val_lstr);
            if (val_lstr.size() > 0 && val_lstr[0] == ',') {
                // ????????????
                val_lstr = val_lstr.substr(1);
            } else if (val_lstr.size() > 0 && val_lstr[0] == ';') {
                // parameter???????????????
                val_lstr = decompose_semicoron_separated_kvlist(val_lstr, transfer_encoding.transfer_codings.back());
            } else {
                // ???????????????
                DXOUT("[KO] unexpected state: \"" << val_lstr.substr(0) << "\"");
                break;
            }
        }
    }
    transfer_encoding.currently_chunked
        = !transfer_encoding.empty() && transfer_encoding.current_coding().coding == "chunked";
    QVOUT(transfer_encoding.transfer_codings.back().coding);
}

void RequestHTTP::ControlParams::determine_content_type(const HeaderHTTPHolder &holder) {
    // A sender that generates a message containing a payload body SHOULD generate a Content-Type header field in that
    // message unless the intended media type of the enclosed representation is unknown to the sender. If a Content-Type
    // header field is not present, the recipient MAY either assume a media type of "application/octet-stream"
    // ([RFC2046], Section 4.5.1) or examine the data to determine its type.
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
    light_string type_str(lct, 0, type_end);
    light_string::size_type subtype_end = lct.find_first_not_of(HTTP::CharFilter::tchar, type_end + 1);
    light_string subtype_str(lct, type_end + 1, subtype_end);
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

void RequestHTTP::ControlParams::determine_connection(const HeaderHTTPHolder &holder) {
    // Connection        = 1#connection-option
    // connection-option = token
    const HeaderHTTPHolder::value_list_type *cons = holder.get_vals(HeaderHTTP::connection);
    if (!cons) {
        return;
    }
    if (cons->size() == 0) {
        DXOUT("[KO] list exists, but it's empty.");
        return;
    }
    for (HeaderHTTPHolder::value_list_type::const_iterator it = cons->begin(); it != cons->end(); ++it) {
        light_string val_lstr = light_string(*it);
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

            // ??????
            byte_string target_str = HTTP::Utils::downcase(target_lstr.str());
            DXOUT("target_str: \"" << target_str << "\"");
            connection.connection_options.push_back(target_str);
            if (target_str == "close") {
                connection.close_ = true;
            } else if (target_str == "keep-alive") {
                connection.keep_alive_ = true;
            }

            // ??????
            val_lstr = val_lstr.substr_after(HTTP::CharFilter::sp, target_lstr.size());
            if (val_lstr.size() == 0) {
                DXOUT("away");
                break;
            }
            // cat(sp_end) ??? "," ??? ";" ?????????????????????
            DXOUT("val_lstr[0]: " << val_lstr[0]);
            if (val_lstr[0] == ',') {
                // ????????????
                val_lstr = val_lstr.substr(1);
            } else {
                // ???????????????
                DXOUT("[KO] unexpected state: \"" << val_lstr.substr(0) << "\"");
                break;
            }
        }
    }
    DXOUT("close: " << connection.will_close() << ", keep-alive: " << connection.will_keep_alive());
}

void RequestHTTP::ControlParams::determine_te(const HeaderHTTPHolder &holder) {
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
    if (!tes) {
        return;
    }
    for (HeaderHTTPHolder::value_list_type::const_iterator it = tes->begin(); it != tes->end(); ++it) {
        light_string val_lstr = light_string(*it);
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

            // ??????
            DXOUT("tc_lstr: \"" << tc_lstr << "\"");
            HTTP::Term::TransferCoding tc = HTTP::Term::TransferCoding::init();
            tc.coding                     = tc_lstr.str();
            te.transfer_codings.push_back(tc);

            // ??????
            val_lstr = val_lstr.substr_after(HTTP::CharFilter::sp, tc_lstr.size());
            if (val_lstr.size() == 0) {
                DXOUT("away");
                break;
            }

            DXOUT("val_lstr[0]: " << val_lstr[0]);
            HTTP::Term::TransferCoding &last_coding = te.transfer_codings.back();
            if (val_lstr[0] == ';') {
                // parameter???????????????
                val_lstr = decompose_semicoron_separated_kvlist(val_lstr, last_coding);
            }

            // rank???????????????, ?????????????????????????????????????????????????????????????????????????????? -> q ??????????????????.
            if (!te.transfer_codings.empty()) {
                HTTP::IDictHolder::parameter_dict::iterator qit = last_coding.parameters.find(HTTP::strfy("q"));
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

            if (val_lstr.size() == 0) {
                DXOUT("away");
                break;
            }
            if (val_lstr[0] == ',') {
                // ????????????
                val_lstr = val_lstr.substr(1);
            } else {
                // ???????????????
                DXOUT("[KO] unexpected state: \"" << val_lstr.substr(0) << "\"");
                break;
            }
        }
    }
}

void RequestHTTP::ControlParams::determine_upgrade(const HeaderHTTPHolder &holder) {
    // Upgrade          = 1#protocol
    // protocol         = protocol-name ["/" protocol-version]
    // protocol-name    = token
    // protocol-version = token
    const HeaderHTTPHolder::value_list_type *elems = holder.get_vals(HeaderHTTP::upgrade);
    if (!elems) {
        return;
    }
    for (HeaderHTTPHolder::value_list_type::const_iterator it = elems->begin(); it != elems->end(); ++it) {
        light_string val_lstr = light_string(*it);
        for (; val_lstr.size() > 0;) {
            val_lstr          = val_lstr.substr_after(HTTP::CharFilter::sp);
            light_string name = val_lstr.substr_while(HTTP::CharFilter::tchar);
            if (name.size() == 0) {
                // ?????????
                DXOUT("[KO] no name");
                break;
            }
            HTTP::Term::Protocol protocol;
            protocol.name = name;
            val_lstr      = val_lstr.substr(name.size());
            if (val_lstr.size() > 0 && val_lstr[0] == '/') {
                // version?????????
                light_string version = val_lstr.substr_while(HTTP::CharFilter::tchar, 1);
                if (version.size() == 0) {
                    // ?????????
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

void RequestHTTP::ControlParams::determine_via(const HeaderHTTPHolder &holder) {
    // Via = 1#( received-protocol RWS received-by [ RWS comment ] )
    // received-protocol = [ protocol-name "/" ] protocol-version
    // received-by       = ( uri-host [ ":" port ] ) / pseudonym
    // pseudonym         = token
    // protocol-name     = token
    // protocol-version  = token
    const HeaderHTTPHolder::value_list_type *elems = holder.get_vals(HeaderHTTP::via);
    if (!elems) {
        return;
    }
    for (HeaderHTTPHolder::value_list_type::const_iterator it = elems->begin(); it != elems->end(); ++it) {
        light_string val_lstr = light_string(*it);
        for (;;) {
            val_lstr = val_lstr.substr_after(HTTP::CharFilter::sp);
            QVOUT(val_lstr);
            if (val_lstr.size() == 0) {
                break;
            }
            const light_string p1 = val_lstr.substr_while(HTTP::CharFilter::tchar);
            if (p1.size() == 0) {
                DXOUT("[KO] zero-width token(1): " << val_lstr.qstr());
                break;
            }
            val_lstr = val_lstr.substr(p1.size());
            light_string p2;
            if (val_lstr.size() > 0 && val_lstr[0] != ' ') {
                if (val_lstr[0] != '/') {
                    DXOUT("[KO] invalid separator: " << val_lstr.qstr());
                    break;
                }
                p2       = val_lstr.substr_while(HTTP::CharFilter::tchar, 1);
                val_lstr = val_lstr.substr(1 + p2.size());
            }
            HTTP::Term::Received r;
            HTTP::Term::Protocol &p = r.protocol;
            if (p2.size() == 0) {
                p.version = p1;
            } else {
                p.name    = p1;
                p.version = p2;
            }
            DXOUT("protocol: name=" << p.name.qstr() << ", version=" << p.version.qstr());
            light_string rws = val_lstr.substr_while(HTTP::CharFilter::sp);
            if (rws.size() >= 1) {
                val_lstr        = val_lstr.substr(rws.size());
                light_string by = val_lstr.substr_before(HTTP::CharFilter::sp | ",");
                DXOUT("by: " << by.qstr());
                bool is_valid_as_uri_host = HTTP::Validator::is_uri_host(by);
                if (!is_valid_as_uri_host) {
                    DXOUT("[KO] not valid as host: " << by.qstr());
                    break;
                }
                pack_host(r.host, by);
                val_lstr = val_lstr.substr(by.size());
            }
            via.receiveds.push_back(r);
            val_lstr = val_lstr.substr_after(HTTP::CharFilter::sp);

            const light_string comment = extract_comment(val_lstr);
            QVOUT(comment);
            QVOUT(val_lstr);

            if (val_lstr.size() > 0 && val_lstr[0] == ',') {
                val_lstr = val_lstr.substr(1);
                continue;
            }
            break;
        }
    }
}

void RequestHTTP::ControlParams::pack_host(HTTP::Term::Host &host_item, const light_string &lhost) {
    host_item.value = lhost.str();
    // ??????????????? lhost ??? Host: ???????????????
    // -> 1???????????? [ ??????????????? ipv6(vfuture) ???????????????????????????.
    if (lhost[0] == '[') {
        // ipv6 or ipvfuture
        const light_string host = lhost.substr_before("]", 1);
        host_item.host          = host.str();
        if (host.size() < lhost.size()) {
            host_item.port = lhost.substr(1 + host.size() + 2).str();
        }
    } else {
        // ipv4 or reg-name
        const light_string host = lhost.substr_before(":");
        host_item.host          = host.str();
        if (host.size() < lhost.size()) {
            light_string port = lhost.substr(host.size() + 1);
            host_item.port    = port.str();
        }
    }
    DXOUT("host: \"" << host_item.host << "\"");
    DXOUT("port: \"" << host_item.port << "\"");
}

RequestHTTP::light_string RequestHTTP::ControlParams::extract_comment(light_string &val_lstr) {
    // comment        = "(" *( ctext / quoted-pair / comment ) ")"
    // ctext          = HTAB / SP / %x21-27 / %x2A-5B / %x5D-7E / obs-text
    //                ; HTAB + SP + ??????????????????, ??????????????????????????????????????????
    if (val_lstr.size() == 0 || val_lstr[0] != '(') {
        return val_lstr.substr(0, 0);
    }
    int paren                 = 0;
    bool quoted               = false;
    light_string comment_from = val_lstr;
    light_string::size_type n = 0;
    for (; val_lstr.size() > 0; ++n, val_lstr = val_lstr.substr(1)) {
        const char c = val_lstr[0];
        if (quoted) {
            if (!HTTP::CharFilter::qdright.includes(c)) {
                DXOUT("[KO] quoting non-quotable char: " << val_lstr.qstr());
                break;
            }
            quoted = false;
        } else {
            if (c == '(') {
                paren += 1;
            } else if (c == ')') {
                if (paren <= 0) {
                    DXOUT("[KO] too much ): " << val_lstr.qstr());
                    break;
                }
                paren -= 1;
                if (paren == 0) {
                    DXOUT("finish comment");
                    val_lstr = val_lstr.substr(1);
                    ++n;
                    break;
                }
            } else if (c == '\\') {
                quoted = true;
            } else if (!HTTP::CharFilter::ctext.includes(c)) {
                DXOUT("[KO] non ctext in comment: " << val_lstr.qstr());
                break;
            }
        }
    }
    return comment_from.substr(0, n);
}

RequestHTTP::light_string
RequestHTTP::ControlParams::decompose_semicoron_separated_kvlist(const light_string &kvlist_str,
                                                                 HTTP::IDictHolder &holder) {
    light_string list_str = kvlist_str;
    // *( OWS ";" OWS parameter )
    for (;;) {
        light_string params_str = list_str;
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
        // ????????????parameter?????????
        // token
        const light_string key_lstr = params_str.substr_while(HTTP::CharFilter::tchar);
        if (key_lstr.size() == params_str.size() || params_str[key_lstr.size()] != '=') {
            DXOUT("[KO] no equal");
            return params_str.substr(params_str.size());
        }
        // [?????????]
        // ????????????case-insensitive
        // https://wiki.suikawiki.org/n/Content-Type#anchor-11
        // > ?????????????????????????????????????????????????????????????????????????????????
        params_str = params_str.substr(key_lstr.size() + 1);

        byte_string key_str = ParserHelper::normalize_header_key(key_lstr);
        DXOUT("key: \"" << key_str << "\"");
        {
            const light_string just_value_str = ParserHelper::extract_quoted_or_token(params_str);
            holder.store_list_item(key_str, just_value_str);
            list_str = params_str.substr(just_value_str.size());
        }
    }
    return list_str;
}

bool RequestHTTP::is_ready_to_navigate() const {
    return this->ps.parse_progress >= PP_BODY;
}

bool RequestHTTP::is_ready_to_respond() const {
    return this->ps.parse_progress >= PP_OVER;
}

size_t RequestHTTP::receipt_size() const {
    return bytebuffer.size();
}

size_t RequestHTTP::parsed_body_size() const {
    return this->mid - this->ps.start_of_body;
}

size_t RequestHTTP::parsed_size() const {
    return this->mid;
}

HTTP::t_version RequestHTTP::get_http_version() const {
    return this->cp.http_version;
}

RequestHTTP::byte_string RequestHTTP::get_body() const {
    if (cp.is_body_chunked) {
        return chunked_body.body();
    } else {
        return byte_string(bytebuffer.begin() + this->ps.start_of_body, bytebuffer.begin() + this->ps.end_of_body);
    }
}

bool RequestHTTP::should_keep_in_touch() const {
    // TODO: ?????????
    return false;
}
