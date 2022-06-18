#include "IndexRange.hpp"

IndexRange::IndexRange():
    pair(0, 0) {}

IndexRange::IndexRange(ssize_t f, ssize_t t):
    pair(f, t) {}

IndexRange  IndexRange::invalid() {
    return IndexRange(0, -1);
}

bool        IndexRange::is_invalid() const {
    return first > second;
}

ssize_t     IndexRange::length() const {
    return second - first;
}

std::ostream&   operator<<(std::ostream& out, const IndexRange& r) {
    return out << "[" << r.first << "," << r.second << ")";
}
