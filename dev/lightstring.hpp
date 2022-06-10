#ifndef LIGHTSTRING_HPP
# define LIGHTSTRING_HPP
# include <string>
# include <algorithm>

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

private:
    const_iterator  first;
    const_iterator  last;

public:
    LightString() {
        first = last;
    }

    LightString(const std::string& str):
        first(str.begin()), last(str.end()) {}

    LightString(const_iterator f, const_iterator l):
        first(f), last(l) {}

    LightString& operator=(const string_class& rhs) {
        first = rhs.begin();
        last = rhs.end();
        return *this;
    }

    string_class    str() const {
        if (first == last) { return ""; }
        return string_class(first, last);
    }

    size_type       size() const {
        return std::distance(first, last);
    }

    size_type       length() const {
        return std::distance(first, last);
    }

    iterator        begin() {
        return first;
    }

    const_iterator  begin() const {
        return first;
    }

    iterator        end() {
        return last;
    }

    const_iterator  end() const {
        return last;
    }

    size_type       find_first_of(const string_class& str, size_type pos = 0) const {
        size_type d = std::distance(first, last);
        if (pos >= d) {
            return std::string::npos;
        }
        const_iterator it = std::find_first_of(begin() + pos, end(), str.begin(), str.end());
        if (it == end()) {
            return std::string::npos;
        }
        return std::distance(first, it);
    }

    size_type       find_first_not_of(const string_class& str, size_type pos = 0) const {
        size_type d = std::distance(first, last);
        if (pos >= d) {
            return std::string::npos;
        }
        if (first == last) {
            return std::string::npos;
        }
        for (const_iterator it = first; it != last; ++it) {
            if (str.find_first_of(*it) == std::string::npos) {
                return std::distance(first, it);
            }
        }
        return std::string::npos;
    }

    size_type       find_last_not_of(const string_class& str, size_type pos = 0) const {
        size_type d = std::distance(first, last);
        if (pos >= d) {
            return std::string::npos;
        }
        if (first == last) {
            return std::string::npos;
        }
        for (const_iterator it = last; --it != first;) {
            if (str.find_first_of(*it) == std::string::npos) {
                return std::distance(first, it);
            }
        }
        return std::string::npos;
    }

    LightString substr(size_type pos = 0, size_type n = std::string::npos) const {
        if (n == std::string::npos) {
            return LightString(first + pos, last);
        }
        size_type   rlen = size() - pos;
        if (rlen < n) {
            rlen = n;
        }
        return LightString(first + pos, first + pos + rlen);
    }
};

#endif
