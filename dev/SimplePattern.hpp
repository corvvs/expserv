#ifndef SIMPLEPATTERN_HPP
#define SIMPLEPATTERN_HPP
#include "IndexRange.hpp"
#include "LightString.hpp"
#include "test_common.hpp"
#include <iostream>
#include <string>
#include <vector>

template <class T>
class SimplePattern {
public:
    typedef T element_type;
    typedef std::basic_string<T> string_type;
    typedef typename string_type::size_type size_type;

private:
    // 生のパターン
    const string_type raw_pattern;
    // 文字
    std::vector<element_type> chars;
    // その位置の文字が省略可能かどうかを 01 で表す
    std::vector<int> omittables;

    size_type len_minimum;
    size_type len_maximum;

    static const element_type spchar_escape    = '\\';
    static const element_type spchar_omittable = '?';

    void set_pattern(const string_type &pattern) {
        chars.clear();
        omittables.clear();
        bool now_escaped = false;
        len_minimum      = 0;
        len_maximum      = 0;
        // DSOUT();
        for (typename string_type::size_type i = 0; i < pattern.length(); ++i) {
            if (pattern[i] == spchar_escape) {
                if (!now_escaped && i + 1 < pattern.length()) {
                    now_escaped = true;
                    continue;
                }
            }
            chars.push_back(pattern[i]);
            if (i + 1 < pattern.length() && pattern[i + 1] == spchar_omittable) {
                ++i;
                omittables.push_back(1);
            } else {
                ++len_minimum;
                omittables.push_back(0);
            }
            ++len_maximum;
            // std::cout << "(" << chars.back() << omittables.back() << ")";
            now_escaped = false;
        }
        // std::cout << std::endl;
    }

public:
    SimplePattern(const string_type &pattern) : raw_pattern(pattern) {
        set_pattern(pattern);
    }

    const string_type &get_pattern() const {
        return raw_pattern;
    }

    IndexRange find(const string_type &target) const {
        return find(LightString<element_type>(target));
    }

    IndexRange find(const LightString<T> &target) const {
        std::vector<int> sizes;
        size_type i = 0;
        for (; i <= target.length(); ++i) {
            sizes.clear();
            size_type j = 0;
            size_type k = 0;
            for (; j < chars.size() && i + k <= target.length(); ++j) {
                if (i + k < target.length() && target[i + k] == chars[j]) {
                    ++k;
                } else if (!omittables[j]) {
                    break;
                }
            }
            if (j < chars.size()) {
                continue;
            }
            return IndexRange(i, i + k);
        }
        return IndexRange(target.length() + 1, target.length());
    }
};

#endif
