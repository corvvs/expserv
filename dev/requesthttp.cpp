#include "requesthttp.hpp"

size_t max_reqline_end = 8192;

t_http_method   judge_http_method(
    RequestHTTP::byte_buffer::iterator begin,
    RequestHTTP::byte_buffer::iterator end
) {
    RequestHTTP::byte_buffer sub(begin, end);
    if (sub == "GET") {
        return HTTP_METHOD_GET;
    }
    if (sub == "POST") {
        return HTTP_METHOD_POST;
    }
    if (sub == "DELETE") {
        return HTTP_METHOD_DELETE;
    }
    throw std::runtime_error("Invalid Request: unsupported method");
}

t_http_version  judge_http_version(
    RequestHTTP::byte_buffer::iterator begin,
    RequestHTTP::byte_buffer::iterator end
) {
    RequestHTTP::byte_buffer sub(begin, end);
    if (sub == "HTTP/1.0") {
        return HTTP_V_1_0;
    }
    if (sub == "HTTP/1.1") {
        return HTTP_V_1_1;
    }
    throw std::runtime_error("Invalid Request: unsupported version");
}

RequestHTTP::RequestHTTP():
    mid(0),
    parse_progress(PARSE_REQUEST_REQLINE_START),
    http_method(HTTP_METHOD_UNKNOWN),
    http_version(HTTP_V_UNKNOWN)
{
    bytebuffer.reserve(max_reqline_end);
    std::cout << "[R] generated" << std::endl;
}

RequestHTTP::~RequestHTTP() {}

bool    RequestHTTP::extract_reqline_start(size_t len) {
    size_t s_o_s = ParserHelper::ignore_crlf(&*(bytebuffer.begin()) + mid, len);
    DOUT() << len << " -> " << s_o_s << std::endl;
    mid += s_o_s;
    if (s_o_s == len) {
        return false;
    }
    // 開始行の開始位置が定まった
    start_of_reqline = mid;
    DOUT() << "start_of_reqline is: " << start_of_reqline << std::endl;
    return true;
}

bool    RequestHTTP::extract_reqline_end(size_t len) {
    ParserHelper::index_range res = ParserHelper::find_crlf(&*(bytebuffer.begin()) + mid, len);
    mid += res.second;
    if (res.first >= res.second) { return false; }
    // CRLFが見つかった
    end_of_reqline = mid - (res.second - res.first);
    start_of_header = mid;
    return true;
}

void    RequestHTTP::parse_reqline() {
    byte_buffer::iterator begin = bytebuffer.begin() + start_of_reqline;
    byte_buffer::iterator end = bytebuffer.begin() + end_of_reqline;
    std::vector< byte_buffer > splitted = ParserHelper::split_by_sp(begin, end);

    switch (splitted.size()) {
        case 2:
        case 3: {
            // HTTP/0.9?
            // HTTP/1.*?

            http_method = judge_http_method(splitted[0].begin(), splitted[0].end());
            DSOUT() << "http_method: " << http_method << std::endl;
            request_path = splitted[1];
            DSOUT() << "request_path: " << request_path << std::endl;
            if (splitted.size() == 3) {
                http_version = judge_http_version(splitted[2].begin(), splitted[2].end());
            } else {
                http_version = HTTP_V_0_9;
            }
            DSOUT() << "http_version: " << http_version << std::endl;
            break;
        }
        default:
            throw std::runtime_error("Invalid Request: invalid request-line?");
    }
}

bool    RequestHTTP::extract_header_end(size_t len) {
    ParserHelper::index_range res = ParserHelper::find_blank_line(&*bytebuffer.begin() + mid, len);
    DSOUT() << "res: [" << res.first << "," << res.second << ")" << std::endl;
    mid += res.second;
    if (res.first >= res.second) { return false; }
    end_of_header = mid - (res.second - res.first);
    start_of_body = mid;
    return true;
}

void    RequestHTTP::feed_bytes(char *bytes, size_t len) {
    bytebuffer.insert(bytebuffer.end(), bytes, bytes + len);
    std::cout << "[R] feed: " << len << std::endl;
    std::cout << "[R] progress: " << parse_progress << std::endl;
    std::cout
        << "```" << std::endl
        << bytebuffer
        << "```" << std::endl;

    switch (parse_progress) {
        // 開始行の開始位置を探す
        case PARSE_REQUEST_REQLINE_START: {
            DSOUT() << "* determining start_of_reqline... *" << std::endl;
            if (!extract_reqline_start(len)) {
                return;
            }
            DSOUT() << "* determined start_of_reqline *" << std::endl;
            parse_progress = PARSE_REQUEST_REQLINE_END;
            len = bytebuffer.length() - mid;
            DSOUT() << "mid: " << mid <<  ", len: " << len << std::endl;
        }

        // 開始行の終了位置を探す
        case PARSE_REQUEST_REQLINE_END: {
            DSOUT() << "* determining end_of_reqline... *" << std::endl;
            if (!extract_reqline_end(len)) {
                return;
            }
            // -> end_of_reqline が8192バイト以内かどうか調べる。
            if (max_reqline_end <= end_of_reqline) {
                throw std::runtime_error("Invalid Response: request line is too long");
            }
            // -> [start_of_reqline, end_of_reqline) が 開始行かどうか調べる.
            DSOUT() << "* determined end_of_reqline *" << std::endl;
            DSOUT() << "\"" << byte_buffer(bytebuffer.begin() + start_of_reqline, bytebuffer.begin() + end_of_reqline) << "\"" << std::endl;
            DSOUT() << "len: " << len << std::endl;
            DSOUT() << "end_of_reqline: " << end_of_reqline << std::endl;
            DSOUT() << "start_of_header: " << start_of_header << std::endl;

            DSOUT() << "* parsing reqline... *" << std::endl;
            parse_reqline();
            DSOUT() << "* parsed reqline *" << std::endl;
            parse_progress = PARSE_REQUEST_HEADER;
            len = bytebuffer.length() - mid;
        }

        // ヘッダの終わりを探す
        case PARSE_REQUEST_HEADER: {
            DSOUT() << "* determining end_of_header... *" << std::endl;
            if (!extract_header_end(len)) {
                DSOUT() << "* not found *" << std::endl;
                return;
            }
            DSOUT() << "* determined end_of_header *" << std::endl;
            DSOUT() << "end_of_header: " << end_of_header << std::endl;
            raw_header = byte_buffer(bytebuffer.begin() + start_of_header, bytebuffer.begin() + end_of_header);
            DSOUT()
                << "```"
                << std::endl
                << raw_header
                << std::endl
                << "```" << std::endl;
            parse_progress = PARSE_REQUEST_BODY;
            len = bytebuffer.length() - mid;
            break;
        }

        case PARSE_REQUEST_BODY: {
            // とりあえずここまで
            parse_progress = PARSE_REQUEST_OVER;
            return;
        }

        default:
            std::runtime_error("not implemented yet");
    }
}
