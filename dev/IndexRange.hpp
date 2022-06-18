#ifndef INDEXRANGE_HPP
# define INDEXRANGE_HPP
# include <string>
# include <utility>
# include <iostream>
# include "test_common.hpp"

struct IndexRange: public std::pair<ssize_t, ssize_t> {
    IndexRange();
    IndexRange(ssize_t f, ssize_t t);
    static IndexRange   invalid();
    bool                is_invalid() const;
    ssize_t             length() const;
};

std::ostream&   operator<<(std::ostream& out, const IndexRange& r);

#endif
