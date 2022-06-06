#ifndef PARSERHELPER_HPP
# define PARSERHELPER_HPP
# include <string>
# include <vector>
# include <utility>
# include <sstream>
# include "http.hpp"
# include "test_common.hpp"

namespace ParserHelper {
    typedef HTTP::byte_string               byte_string;
    typedef std::pair<ssize_t, ssize_t>     index_range;

    const byte_string SP = " ";
    const byte_string OWS = " \t";
    const byte_string HEADER_KV_SPLITTER = ":";
    const byte_string CRLF = "\r\n";
    const byte_string LF = "\n";

    // LF, または CRLF を見つける.
    // 見つかった場合, LFまたはCRLFの [開始位置, 終了位置の次) のペアを返す.
    // 見つからない場合, [len, len) を返す.
    index_range find_crlf(const byte_string& str, ssize_t from, ssize_t len);

    // 空白行、すなわち「LFまたはCRLF」が2つ連続している部分を見つける.
    index_range find_blank_line(const byte_string& str, ssize_t from, ssize_t len);

    // 文字列の先頭から, CRLRおよびLFをすべてスキップした位置のインデックスを返す
    ssize_t     ignore_crlf(const byte_string& str, ssize_t from, ssize_t len);
    bool        is_sp(char c);

    // 文字列の先頭から, 「空白」をすべてスキップした位置のインデックスを返す
    ssize_t     ignore_sp(const byte_string& str, ssize_t from, ssize_t len);

    // 文字列の先頭から, 「空白」以外をすべてスキップした位置のインデックスを返す
    ssize_t     ignore_not_sp(const byte_string& str, ssize_t from, ssize_t len);

    // 文字列を「空白」で分割する
    std::vector< byte_string >  split_by_sp(
        byte_string::const_iterator first,
        byte_string::const_iterator last
    );

    // HTTPヘッダ key が「リスト」かどうかを返す
    bool        is_listing_header(const byte_string& key);

    // string to size_t 変換
    unsigned int    stou(const byte_string& str);
    byte_string     utos(unsigned int u);
}

#endif
