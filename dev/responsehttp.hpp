#ifndef RESPONSEHTTP_HPP
# define RESPONSEHTTP_HPP
# include <string>
# include <vector>
# include "http.hpp"
# include "http_error.hpp"
# include "parserhelper.hpp"

class ResponseHTTP {
public:
    typedef HTTP::byte_string       byte_string;
    typedef HTTP::header_dict_type  header_dict_type;

private:
    HTTP::t_version     version_;
    HTTP::t_status      status_;
    bool                is_error;
    size_t              consumed;

    std::vector<HTTP::header_kvpair_type>
                        header_list;
    header_dict_type    header_dict;
    byte_string         body;

    byte_string         message_text;

public:
    // 通常(エラーでない)応答を構築する
    ResponseHTTP(
        HTTP::t_version version,
        HTTP::t_status status
    );

    // エラー応答を構築する
    ResponseHTTP(
        HTTP::t_version version,
        http_error error
    );

    // HTTPヘッダを追加する
    void    feed_header(
        const HTTP::header_key_type& key,
        const HTTP::header_val_type& val);

    void    feed_body(const byte_string& str);

    // 保持している情報をもとにHTTPメッセージのテキストデータを生成し, 
    // message_text に入れる.
    void    render();

    const byte_string&  get_message_text() const;
    const char*         get_unsent() const;
    void                mark_sent(size_t sent);
    size_t              get_unsent_size() const;
};

#endif
