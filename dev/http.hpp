#ifndef HTTP_HPP
# define HTTP_HPP
# include <string>
# include <map>

// 全体で共通して使うenum, 型, 定数, フリー関数など

namespace HTTP {
    // ステータスコード
    enum t_status {
        STATUS_OK                       = 200,

        STATUS_FOUND                    = 302,

        STATUS_BAD_REQUEST              = 400,
        STATUS_UNAUTHORIZED             = 401,
        STATUS_FORBIDDEN                = 403,
        STATUS_NOT_FOUND                = 404,
        STATUS_METHOD_NOT_ALLOWED       = 405,
        STATUS_URI_TOO_LONG             = 414,
        STATUS_IM_A_TEAPOT              = 418,

        STATUS_INTERNAL_SERVER_ERROR    = 500,
        STATUS_NOT_IMPLEMENTED          = 501,
        STATUS_BAD_GATEWAY              = 502,
        STATUS_SERVICE_UNAVAILABLE      = 503,
        STATUS_VERSION_NOT_SUPPORTED    = 505,

        STATUS_DUMMY = 0
    };

    // リクエストメソッド
    enum t_method {
        METHOD_UNKNOWN,

        METHOD_GET,
        METHOD_POST,
        METHOD_DELETE,

        METHOD_ERROR
    };

    // HTTPバージョン
    enum t_version {
        V_UNKNOWN,

        V_0_9,
        V_1_0,
        V_1_1,

        V_ERROR
    };

    typedef char                            byte_type;
    // バイト列
    typedef std::basic_string<byte_type>    byte_string;
    // ヘッダのキーの型
    typedef byte_string                     header_key_type;
    // ヘッダの値の型
    typedef byte_string                     header_val_type;
    // ヘッダのキー・値の組
    typedef std::pair<header_key_type, header_val_type>
                                            header_kvpair_type;
    // ヘッダを格納する辞書
    typedef std::map<header_key_type, header_val_type>
                                            header_dict_type;

    // サーバのデフォルトのHTTPバージョン
    const t_version     DEFAULT_HTTP_VERSION = V_1_1;

    const byte_string   version_str(t_version version);
    const byte_string   reason(t_status status);

    // 文字集合
    namespace Charset {

        // アルファベット・小文字
        const byte_string alpha_low = "abcdefghijklmnopqrstuvwxyz";
        // アルファベット・大文字
        const byte_string alpha_up = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        // アルファベット
        const byte_string alpha = alpha_low + alpha_up;
        // 数字
        const byte_string digit = "0123456789";
        // 16進数における数字
        const byte_string hexdig = digit + "abcdef" + "ABCDEF";
        // HTTPにおける非予約文字
        const byte_string unreserved = alpha + digit + "-._~";
        const byte_string gen_delims = ":/?#[]@";
        const byte_string sub_delims = "!$&'()*+.;=";
    }

    // 単純な文字集合クラス
    class CharFilter {
    private:
        uint64_t    filter[4];

    public:
        CharFilter(const byte_string& chars);
        CharFilter(const char* chars);
        CharFilter(const CharFilter& other);

        CharFilter& operator=(const CharFilter& rhs);
        CharFilter& operator=(const byte_string& rhs);

        CharFilter  operator|(const CharFilter& rhs) const;
        CharFilter  operator&(const CharFilter& rhs) const;
        CharFilter  operator^(const CharFilter& rhs) const;

        void        fill(const byte_string& chars);
        // `c` が文字集合に含まれるかどうか
        bool        includes(uint8_t c) const;

        // アルファベット・小文字
        static const CharFilter alpha_low;
        // アルファベット・大文字
        static const CharFilter alpha_up;
        // アルファベット
        static const CharFilter alpha;
        // 数字
        static const CharFilter digit;
        // 16進数における数字
        static const CharFilter hexdig;
        // HTTPにおける非予約文字
        static const CharFilter unreserved;
        static const CharFilter gen_delims;
        static const CharFilter sub_delims;
    };
}

#endif
