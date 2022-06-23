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
            HTTP::byte_string   value;
            HTTP::byte_string   port;
            HTTP::byte_string   host;
        };

        // Transfer-Encoding
        struct TransferEncoding {
            std::vector<HTTP::Term::TransferCoding>   tranfer_codings;

            bool                        empty() const;
            const Term::TransferCoding& curr_coding() const;
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
