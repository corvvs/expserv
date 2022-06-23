#ifndef HEADERITEMHTTP_HPP
# define HEADERITEMHTTP_HPP
# include "http.hpp"
# include "LightString.hpp"
# include <string>
# include <list>
# include <map>
# include "parserhelper.hpp"

namespace HeaderHTTP {
    const std::string host              = "host";
    const std::string connection        = "connection";
    const std::string cookie            = "cookie";
    const std::string set_cookie        = "set-cookie";
    const std::string pragma            = "pragma";
    const std::string user_agent        = "user_agent";
    const std::string cache_control     = "cache-control";
    const std::string authorization     = "authorization";
    const std::string www_authenticate  = "www-authenticate";
    const std::string keep_alive        = "keep-alive";
    const std::string content_type      = "content-type";
    const std::string content_length    = "content-length";
    const std::string transfer_encoding = "transfer-encoding";
    const std::string te                = "te";
    const std::string vary              = "vary";
}

// あるヘッダキーの属性
struct HeaderHTTPAttribute {
    typedef HTTP::header_key_type   header_key_type;
    typedef std::map<header_key_type, HeaderHTTPAttribute>
                                    attr_dict_type;

    // [属性]

    // リスト値である
    bool    is_list;
    // 複数の同名ヘッダを1つにまとめてよい
    bool    is_aggregatable;
    // 複数指定されていてはならない
    bool    must_be_unique;

    // 定義済み属性
    static attr_dict_type   predefined_attrs;
    // 定義済み属性を設定する
    static void set_predefined_attrs();
};

// ヘッダーのkeyとvalue(s)を保持する
// 再確保が起きないコンテナ(list)で保持し, mapにポインタを保持する
class HeaderHTTPItem {
public:
    typedef HTTP::header_key_type       header_key_type;
    typedef HTTP::header_val_type       header_val_type;
    // ここのlistはvectorでいいかも?
    typedef std::list<header_val_type>  value_list_type;

private:
    const header_key_type   key;
    value_list_type         values;
    HeaderHTTPAttribute     attr;

public:
    // キーを与えて構築
    HeaderHTTPItem(const header_key_type& key);

    // 値を与える
    void    add_val(const header_val_type& val);

    const header_val_type*  get_val() const;
    const header_val_type*  get_back_val() const;
    const value_list_type&  get_vals() const;
};

// HeaderHTTPItem を保持する
class HeaderHTTPHolder {
public:
    typedef HTTP::header_key_type                       header_key_type;
    typedef HTTP::header_val_type                       header_val_type;
    typedef LightString<HTTP::byte_type>                light_string;
    typedef HeaderHTTPItem                              header_item_type;
    // なぜ vector などではなく list を使うのかというと, 再確保を防ぐため.
    // 再確保を防ぐのは, dict で HeaderHTTPItem のポインタを保持するから.
    typedef std::list<HeaderHTTPItem>                   list_type;
    // operator[]
    // d["host"]
    typedef std::map<header_key_type, HeaderHTTPItem*>  dict_type;
    typedef HeaderHTTPItem::value_list_type             value_list_type;

private:
    list_type   list;
    dict_type   dict;

public:
    // 指定したキーに値を追加する
    void                    add_item(const light_string& key, const header_val_type& val);
    // 指定したキーの値オブジェクトを取得する
    const header_item_type* get_item(const header_key_type& normalized_key) const;
    // 指定したキーの値を取得する; 複数ある場合は先頭を取得する
    const header_val_type*  get_val(const header_key_type& normalized_key) const;
    // 指定したキーの末尾の値を取得する
    const header_val_type*  get_back_val(const header_key_type& normalized_key) const;
    // 指定したキーの値をすべて取得する
    const value_list_type*  get_vals(const header_key_type& normalized_key) const;
};

#endif
