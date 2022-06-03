#include "parserhelper.hpp"

// LF, または CRLF を見つける.
// 見つかった場合, LFまたはCRLFの [開始位置, 終了位置の次) のペアを返す.
// 見つからない場合, [len, len) を返す.
ParserHelper::index_range ParserHelper::find_crlf(char *bytes, ssize_t len) {
    for (ssize_t i = 0; i < len; i++) {
        if (bytes[i] == '\n') {
            if (0 < i && bytes[i - 1] == '\r') {
                return index_range(i - 1, i + 1);
            }
            return index_range(i, i + 1);
        }
    }
    return index_range(len, len);
}

// 空白行、すなわち「LFまたはCRLF」が2つ連続している部分を見つける.
ParserHelper::index_range ParserHelper::find_blank_line(char *bytes, ssize_t len) {
    ssize_t i;
    for(i = 0; i < len;) {
        DOUT() << "i: " << i << std::endl;
        index_range nl = find_crlf(bytes + i, len - i);
        DOUT() << "nl: [" << nl.first << "," << nl.second << ")" << std::endl;
        if (nl.second == 0) {
            break;
        }
        i += nl.second;
        if (i + 1 < len && bytes[i + 1] == '\r') {
            i += 1;
        }
        if (i + 1 < len && bytes[i + 1] == '\n') {
            return index_range(nl.first, i + 1);
        }
    }
    return index_range(len, len);
}

// 文字列の先頭から, CRLRおよびLFをすべてスキップした位置のインデックスを返す
ssize_t      ParserHelper::ignore_crlf(char *bytes, ssize_t len) {
    ssize_t i = 0;
    for (; i < len; i++) {
        DOUT() << i << ": " << bytes[i] << std::endl;
        if (bytes[i] == '\n') {
            continue;
        } else if (bytes[i] == '\r') {
            if (i + 1 < len && bytes[i] == '\n') {
                continue;
            }
        }
        break;
    }
    DOUT() << "result: " << i << std::endl;
    return i;
}

bool        ParserHelper::is_sp(char c) {
    return c == ' ';
}

// 文字列の先頭から, 「空白」をすべてスキップした位置のインデックスを返す
ssize_t      ParserHelper::ignore_sp(char *bytes, ssize_t len) {
    ssize_t i = 0;
    for (; i < len; i++) {
        if (is_sp(bytes[i])) {
            continue;
        }
        break;
    }
    return i;
}

// 文字列の先頭から, 「空白」以外をすべてスキップした位置のインデックスを返す
ssize_t      ParserHelper::ignore_not_sp(char *bytes, ssize_t len) {
    ssize_t i = 0;
    for (; i < len; i++) {
        if (!is_sp(bytes[i])) {
            continue;
        }
        break;
    }
    return i;
}

// 文字列を「空白」で分割する
std::vector< ParserHelper::byte_buffer >  ParserHelper::split_by_sp(
    ParserHelper::byte_buffer::iterator first,
    ParserHelper::byte_buffer::iterator last
) {
    typedef std::basic_string<char> str_type;
    std::vector< str_type >  rv;
    str_type::size_type len = std::distance(first, last);
    str_type::size_type i = 0;
    str_type::size_type word_from = 0;
    bool prev_is_sp = true;
    for (; i <= len; i++) {
        bool cur_is_sp = i == len || is_sp(*(first + i));
        if (cur_is_sp && !prev_is_sp) {
            rv.push_back(str_type(
                first + word_from, first + i
            ));
        } else if (!cur_is_sp && prev_is_sp) {
            word_from = i;
        }
        prev_is_sp = cur_is_sp;
    }
    return rv;
}
