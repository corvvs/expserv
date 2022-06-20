#ifndef LIGHTSTRING_HPP
# define LIGHTSTRING_HPP
# include <string>
# include <algorithm>
# include "IndexRange.hpp"

const std::string blank_str = "";

// 別のstringの一部分をiteratorペアとして参照する軽量string
// C++17以降にある string_view と思えば良いか
template <class T>
class LightString {
public:

    typedef T                                       element;
    typedef std::basic_string<T>                    string_class;
    typedef typename string_class::iterator         iterator;
    typedef typename string_class::const_iterator   const_iterator;
    typedef typename string_class::size_type        size_type;
    static const typename string_class::size_type   npos = string_class::npos;
    typedef typename string_class::reference        reference;
    typedef typename string_class::const_reference  const_reference;

private:

    const string_class& base;
    size_type           first;
    size_type           last;

public:

    LightString(): base(blank_str) {
        first = last;
    }

    LightString(const string_class& str):
        base(str), first(0), last(str.length()) {}

    LightString(const string_class& str, const_iterator f, const_iterator l):
        base(str), first(std::distance(str.begin(), f)), last(std::distance(str.begin(), l)) {}

    LightString(const string_class& str, size_type fi, size_type li):
        base(str), first(fi), last(li) {}

    LightString(const string_class& str, const IndexRange& range):
        base(str), first(range.first), last(range.second) {}

    LightString(const LightString& lstr, size_type fi, size_type li):
        base(lstr.base), first(lstr.first + fi), last(lstr.first + li) {}

    LightString& operator=(const LightString& rhs) {
        const_cast<string_class&>(base) = rhs.base;
        first = rhs.first;
        last = rhs.last;
        return *this;
    }

    LightString& operator=(const string_class& rhs) {
        const_cast<string_class&>(base) = rhs;
        first = 0;
        last = rhs.length();
        return *this;
    }

    // 参照先文字列を取得
    const string_class& get_base() const {
        return base;
    }

    // std::string を生成
    string_class    str() const {
        if (first == last) { return ""; }
        return string_class(base.begin() + first, base.begin() + last);
    }

    size_type       size() const {
        return last - first;
    }

    size_type       length() const {
        return last - first;
    }

    iterator        begin() {
        return base.begin() + first;
    }

    const_iterator  begin() const {
        return base.begin() + first;
    }

    iterator        end() {
        return base.begin() + last;
    }

    const_iterator  end() const {
        return base.begin() + last;
    }

    const_reference operator[](size_type pos) const {
        return base[first + pos];
    }

    reference       operator[](size_type pos) {
        return base[first + pos];
    }

    // `str` に含まれる文字が(LightString内で)最初に出現する位置を返す
    // `pos`が指定された場合, 位置`pos`以降のみを検索する
    // 位置は参照先文字列ではなく LightString 先頭からの相対位置
    size_type       find_first_of(const string_class& str, size_type pos = 0) const {
        size_type d = length();
        if (pos >= d) {
            return npos;
        }
        const_iterator it = std::find_first_of(begin() + pos, end(), str.begin(), str.end());
        if (it == end()) {
            return npos;
        }
        return std::distance(begin(), it);
    }

    // `str` に含まれる文字が(LightString内で)最後に出現する位置を返す
    // `pos`が指定された場合, 位置`pos`以降のみを検索する
    // 位置は参照先文字列ではなく LightString 先頭からの相対位置
    size_type       find_last_of(const string_class& str, size_type pos = 0) const {
        size_type d = length();
        if (pos >= d) {
            return npos;
        }
        HTTP::CharFilter   filter(str);
        DSOUT() << first + pos << ", " << last << std::endl;
        for (typename string_class::size_type i = last; first + pos < i;) {
            --i;
            if (filter.includes(base[i])) {
                return i - first;
            }
        }
        return npos;
    }

    // `str` に含まれない文字が(LightString内で)最初に出現する位置を返す
    // `pos`が指定された場合, 位置`pos`以降のみを検索する
    // 位置は参照先文字列ではなく LightString 先頭からの相対位置
    size_type       find_first_not_of(const string_class& str, size_type pos = 0) const {
        size_type d = length();
        if (pos >= d) {
            return npos;
        }
        if (first == last) {
            return npos;
        }
        for (const_iterator it = begin(); it != end(); ++it) {
            if (str.find_first_of(*it) == std::string::npos) {
                return std::distance(begin(), it);
            }
        }
        return npos;
    }

    // `str` に含まれる文字が(LightString内で)最後に出現する位置を返す
    // `pos`が指定された場合, 位置`pos`以降のみを検索する
    // 位置は参照先文字列ではなく LightString 先頭からの相対位置
    size_type       find_last_not_of(const string_class& str, size_type pos = 0) const {
        size_type d = length();
        if (pos >= d) {
            return npos;
        }
        if (first == last) {
            return npos;
        }
        for (const_iterator it = end(); --it != begin();) {
            if (str.find_first_of(*it) == std::string::npos) {
                return std::distance(begin(), it);
            }
        }
        return npos;
    }

    // `str`が最初に出現する位置(= 先頭文字のインデックス)を返す
    // `pos`が指定された場合, 位置`pos`以降のみを検索する
    // 位置は参照先文字列ではなく LightString 先頭からの相対位置
    size_type       find(const string_class& str, size_type pos = 0) const {
        for (size_type i = pos; i + str.size() <= size(); ++i) {
            size_type j = 0;
            for (; j < str.size() && operator[](i + j) == str[j]; ++j);
            if (j == str.size()) {
                return i;
            }
        }
        return npos;
    }

    // `str`が最後に出現する位置(= 先頭文字のインデックス)を返す
    // `pos`が指定された場合, 位置`pos`以降のみを検索する
    // 位置は参照先文字列ではなく LightString 先頭からの相対位置
    size_type       rfind(const string_class& str, size_type pos = 0) const {
        for (size_type i = size() - str.size(); pos <= i;) {
            size_type j = 0;
            for (; j < str.size() && operator[](i + j) == str[j]; ++j);
            if (j == str.size()) {
                return i;
            }
            if (pos == i) { break; }
            --i;
        }
        return npos;
    }

    // 指定した位置`pos`から始まる(最大)長さ`n`の区間を参照する LightString を生成して返す
    // `n`を指定しなかった場合, 新たな LightString の終端は現在の終端と一致する
    // 位置は参照先文字列ではなく LightString 先頭からの相対位置
    LightString substr(size_type pos = 0, size_type n = std::string::npos) const {
        if (n == std::string::npos) {
            return LightString(base, begin() + pos, end());
        }
        size_type   rlen = size() - pos;
        if (n < rlen) { // pos + n < size()
            rlen = n;
        }
        DSOUT() << "n = " << n << std::endl;
        DSOUT() << pos << ", " << pos + rlen << std::endl;
        return LightString(base, begin() + pos, begin() + pos + rlen);
    }
};

namespace HTTP {
    typedef LightString<byte_type>  light_string;
}

template <class T>
std::ostream&   operator<<(std::ostream& out, const LightString<T>& ls) {
    return out << typename LightString<T>::string_class(ls.begin(), ls.end());
}

#endif
