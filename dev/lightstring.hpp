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

    size_type       find_first_of(const string_class& str, size_type pos = 0) const {
        size_type d = length();
        if (pos >= d) {
            return std::string::npos;
        }
        const_iterator it = std::find_first_of(begin() + pos, end(), str.begin(), str.end());
        if (it == end()) {
            return std::string::npos;
        }
        return std::distance(begin(), it);
    }

    size_type       find_first_not_of(const string_class& str, size_type pos = 0) const {
        size_type d = length();
        if (pos >= d) {
            return std::string::npos;
        }
        if (first == last) {
            return std::string::npos;
        }
        for (const_iterator it = begin(); it != end(); ++it) {
            if (str.find_first_of(*it) == std::string::npos) {
                return std::distance(begin(), it);
            }
        }
        return std::string::npos;
    }

    size_type       find_last_not_of(const string_class& str, size_type pos = 0) const {
        size_type d = length();
        if (pos >= d) {
            return std::string::npos;
        }
        if (first == last) {
            return std::string::npos;
        }
        for (const_iterator it = end(); --it != begin();) {
            if (str.find_first_of(*it) == std::string::npos) {
                return std::distance(begin(), it);
            }
        }
        return std::string::npos;
    }

    LightString substr(size_type pos = 0, size_type n = std::string::npos) const {
        if (n == std::string::npos) {
            return LightString(base, begin() + pos, end());
        }
        size_type   rlen = size() - pos;
        if (rlen < n) {
            rlen = n;
        }
        return LightString(base, begin() + pos, begin() + pos + rlen);
    }
};

template <class T>
std::ostream&   operator<<(std::ostream& out, const LightString<T>& ls) {
    return out << typename LightString<T>::string_class(ls.begin(), ls.end());
}

#endif
