#ifndef CONTROLHEADERHTTP_HPP
# define CONTROLHEADERHTTP_HPP
# include "http.hpp"
# include "lightstring.hpp"
# include <vector>
# include <map>

namespace HTTP {
    class IDictHolder {
    public:
        typedef std::string         parameter_key_type;
        typedef HTTP::light_string  parameter_value_type;
        typedef std::map<parameter_key_type,
                                    parameter_value_type> parameter_dict;
        virtual ~IDictHolder() {}

        // key と value を受け取って何かする関数
        // 何をするかは実装クラス次第
        virtual void store_list_item(const parameter_key_type& key, const parameter_value_type& val) = 0;
    };

    namespace Term {
        struct TransferCoding: public IDictHolder {
            byte_string                 coding;
            IDictHolder::parameter_dict parameters;
            void store_list_item(const parameter_key_type& key, const parameter_value_type& val);
        };
    }

    // Control-Header
    namespace CH {
        typedef std::string         parameter_key_type;
        typedef HTTP::light_string  parameter_value_type;
        typedef std::map<parameter_key_type,
                                    parameter_value_type> parameter_dict;

        // Host
        struct Host {
            // Host: の値全体
            HTTP::byte_string   value;
            // Host: のホスト部分
            HTTP::byte_string   host;
            // Host: のポート部分; 未指定なら空文字列
            HTTP::byte_string   port;
        };

        // Transfer-Encoding
        struct TransferEncoding {
            // 指定されたTransferCodingが登場順に入る.
            std::vector<HTTP::Term::TransferCoding> tranfer_codings;
            // 現在のTransferCodingが "chunked" かどうか.
            bool                                    currently_chunked;

            // 指定がないかどうか
            bool                        empty() const;
            // 現在のTransferCoding; empty() == true の時に呼び出してはならない.
            const Term::TransferCoding& current_coding() const;
        };

        // Content-Type
        struct ContentType: public IDictHolder {

            HTTP::byte_string   value;
            parameter_dict      parameters;

            // "application/octet-stream"
            static const HTTP::byte_string  default_value;

            void    store_list_item(const parameter_key_type& key, const parameter_value_type& val);
        };
    }
}

#endif
