#include "test_common.hpp"
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace CFG {
typedef std::string string_type;
typedef std::string element_id_type;
typedef string_type::size_type size_type;
const size_type npos = string_type::npos;

class Grammar {
public:
    class Element {
    public:
        Grammar &grammar;
        element_id_type id;
        bool charset[256];
        std::vector<element_id_type> subsidiaries;

        enum Mode { CONCAT, SELECTION, REPETITION };

        size_type le;
        size_type ge;
        Mode mode;
        bool is_surrogate;

        Element(Grammar &grammar, element_id_type id, size_type ge, size_type le, Mode mode, bool is_surrogate)
            : grammar(grammar), id(id), le(le), ge(ge), mode(mode), is_surrogate(is_surrogate) {}

        Element(const Element &other) : grammar(other.grammar) {
            *this = other;
        }

        Element &operator=(const Element &rhs) {
            // DSOUT() << this << ": " << id << " <- " << rhs.id << std::endl;
            id = rhs.id;
            // DSOUT() << this << ": " << id << std::endl;
            subsidiaries = rhs.subsidiaries;
            le           = rhs.le;
            ge           = rhs.ge;
            mode         = rhs.mode;
            is_surrogate = rhs.is_surrogate;
            memcpy(charset, rhs.charset, sizeof(bool) * 256);
            return *this;
        }

        static Element
        def_charset(Grammar &grammar, element_id_type id, const string_type &cs, size_type ge = 1, size_type le = 1) {
            Element el(grammar, id, ge, le, REPETITION, false);
            memset(el.charset, 0, sizeof(bool) * 256);
            for (string_type::size_type i = 0; i < cs.length(); ++i) {
                el.charset[cs[i] % 256] = true;
            }
            return el;
        }

        static Element def_charset(Grammar &grammar,
                                   element_id_type id,
                                   unsigned char first,
                                   unsigned char last,
                                   size_type ge = 1,
                                   size_type le = 1) {
            Element el(grammar, id, ge, le, REPETITION, false);
            memset(el.charset, 0, sizeof(bool) * 256);
            for (; first <= last; ++first) {
                el.charset[first] = true;
            }
            return el;
        }

        static Element def_selection(Grammar &grammar, element_id_type id) {
            Element el(grammar, id, 1, 1, SELECTION, false);
            return el;
        }

        static Element def_concat(Grammar &grammar, element_id_type id) {
            Element el(grammar, id, 1, 1, CONCAT, false);
            return el;
        }

        static Element def_repetition(Grammar &grammar, element_id_type id, size_type ge, size_type le) {
            Element el(grammar, id, ge, le, REPETITION, false);
            return el;
        }

        // predicate: ??????????????????
        bool is_terminal() const {
            return mode == REPETITION && subsidiaries.size() == 0;
        }

        bool is_repetition() const {
            return mode == REPETITION && subsidiaries.size() != 0;
        }

        bool is_selection() const {
            return !is_terminal() && !is_repetition() && mode == SELECTION;
        }

        bool is_concat() const {
            return !is_terminal() && !is_repetition() && mode == CONCAT;
        }

        Element *actual() {
            element_map_type::iterator it = grammar.element_map.find(id);
            if (it == grammar.element_map.end()) {
                DSOUT() << "!!" << id << std::endl;
                throw std::runtime_error("no element for a given id");
            }
            return &it->second;
        }

        Element *nth_actual(size_type i) {
            element_id_type id            = subsidiaries[i];
            element_map_type::iterator it = grammar.element_map.find(id);
            if (it == grammar.element_map.end()) {
                DSOUT() << "!!" << id << std::endl;
                throw std::runtime_error("no element for a given id");
            }
            return &it->second;
        }

        Element *front_actual() {
            return nth_actual(0);
        }

        Element *back_actual() {
            return nth_actual(subsidiaries.size() - 1);
        }

        Element &append(Element el) {
            this->actual()->subsidiaries.push_back(el.id);
            return *this;
        }

        Element &append(element_id_type eid) {
            this->actual()->subsidiaries.push_back(eid);
            return *this;
        }
    };

    typedef std::map<element_id_type, Element> element_map_type;
    element_map_type element_map;

    struct Result {
        const string_type &target;
        Element *elem;
        size_type from_;
        size_type to_;
        std::vector<Result> inners;

        bool is_unmatch() const {
            return from_ > to_;
        }

        bool is_zerowidth() const {
            return !is_unmatch() && from_ == to_;
        }

        Result(const string_type &target, Element *elem, size_type from, size_type to)
            : target(target), elem(elem), from_(from), to_(to) {}

        Result(const Result &other) : target(other.target), elem(other.elem) {
            *this = other;
        }

        Result &operator=(const Result &rhs) {
            elem   = rhs.elem;
            from_  = rhs.from_;
            to_    = rhs.to_;
            inners = rhs.inners;
            return *this;
        }

        static Result unmatch(const string_type &target, Element *elem) {
            return Result(target, elem, 1, 0);
        }

        string_type str() const {
            if (is_unmatch()) {
                return "*unmatched*";
            }
            return string_type(target.begin() + from_, target.begin() + to_);
        }

        friend std::ostream &operator<<(std::ostream &out, CFG::Grammar::Result result) {
            return out << "{" << result.elem->id << ":[" << result.from_ << "," << result.to_ << ")-"
                       << result.inners.size() << "} \""
                       << (result.from_ <= result.to_
                               ? std::string(result.target.begin() + result.from_, result.target.begin() + result.to_)
                               : "*unmatched*")
                       << "\"";
        }
    };

    class Matcher {
    public:
        enum t_phase { NOT_STARTED, NTH_RESULT, FINISHED, DUMMY };

        Element *elem;
        t_phase phase_;
        size_type from;
        size_type el_index;
        std::vector<size_type> tos;
        std::vector<Matcher> matchers;
        std::vector<Result> results;

        Matcher(Element *elem, size_type from) : elem(elem), phase_(NOT_STARTED), from(from) {
            // DSOUT() << from->id << std::endl;
        }

        Matcher &operator=(const Matcher &rhs) {
            elem     = rhs.elem;
            phase_   = rhs.phase_;
            from     = rhs.from;
            el_index = rhs.el_index;
            tos      = rhs.tos;
            matchers = rhs.matchers;
            return *this;
        }

        Result match_phase(const string_type &target, size_type from, size_type depth) {
            Element *actual = elem->actual();
            if (actual->is_terminal()) {

                if (phase_ == NOT_STARTED) {
                    size_type j     = from;
                    size_type count = 0;
                    if (actual->ge <= count && count <= actual->le) {
                        tos.push_back(j);
                    }
                    for (; j < target.length() && count < actual->le;) {
                        if (!actual->charset[(unsigned char)(target[j])]) {
                            break;
                        }
                        ++count;
                        if (actual->ge <= count && count <= actual->le) {
                            tos.push_back(j + 1);
                        }
                        j += 1;
                    }
                    if (tos.empty()) {
                        phase_ = FINISHED;
                    } else {
                        phase_ = NTH_RESULT;
                    }
                }
                switch (phase_) {
                    case NTH_RESULT: {
                        size_type j = tos.back();
                        tos.pop_back();
                        return Result(target, actual, from, j);
                    }
                    case FINISHED:
                        break;
                    default:
                        throw std::runtime_error("unexpected state");
                }

            } else if (actual->is_repetition()) {

                if (phase_ == NOT_STARTED) {
                    if (actual->subsidiaries.empty()) {
                        phase_ = FINISHED;
                    } else {
                        phase_   = NTH_RESULT;
                        el_index = 0;
                        matchers.push_back(Matcher(actual->front_actual(), from));
                        DSOUT() << std::string(depth * 2, ' ') << actual->id << ": matchers++ "
                                << matchers.back().elem->id << std::endl;
                    }
                }
                switch (phase_) {
                    case NTH_RESULT: {
                        // ??????
                        DSOUT() << std::string(depth * 2, ' ') << "[" << actual->ge << ", " << actual->le
                                << "], matchers: " << matchers.size() << std::endl;
                        for (; true;) {
                            size_type f = results.empty() ? from : results.back().to_;
                            Result r    = matchers.back().match(target, depth + 1);
                            DSOUT() << std::string(depth * 2, ' ') << actual->id << ": returned: " << r << std::endl;
                            // [???????????????????????????????????????????????????]
                            // - ??????????????????????????????????????????????????????
                            //   -> ????????????????????????????????????
                            // - ??????????????????????????????(ge)?????????????????????????????????
                            // ???
                            // - ge + 1??????????????????????????????????????????????????????????????????
                            // - ???????????????2???????????????????????????????????????????????????????????????????????????????????????????????????????????????

                            // r ??? el_index + 1 ?????????????????????
                            bool is_re_zero = r.is_zerowidth() && !r.is_unmatch() && !results.empty()
                                              && !results.back().is_zerowidth();
                            bool is_too_much_zero = r.is_zerowidth() && elem->ge < el_index;
                            results.push_back(r);
                            DSOUT() << std::string(depth * 2, ' ') << actual->id << ": results++ "
                                    << results.back().elem->id << std::endl;
                            DSOUT() << std::string(depth * 2, ' ') << r.is_unmatch() << is_re_zero << is_too_much_zero
                                    << std::endl;
                            if (r.is_unmatch() || is_re_zero || is_too_much_zero) {
                                // ????????????????????????
                                DSOUT() << std::string(depth * 2, ' ') << actual->id << ": matchers-- "
                                        << matchers.back().elem->id << std::endl;
                                matchers.pop_back();
                                DSOUT() << std::string(depth * 2, ' ') << actual->id << ": results-- "
                                        << results.back().elem->id << std::endl;
                                results.pop_back();
                                if (el_index == 0) {
                                    results.clear();
                                    if (elem->ge <= el_index && el_index <= elem->le) {
                                        // ?????????????????????????????????????????????
                                        DSOUT() << std::string(depth * 2, ' ') << "(T_T)" << std::endl;
                                        return Result(target, actual, from, from);
                                    }
                                    return Result::unmatch(target, actual);
                                } else {
                                    if (elem->ge <= el_index && el_index <= elem->le) {
                                        Result r(target, actual, from, results.back().to_);
                                        DSOUT() << std::string(depth * 2, ' ') << "results.back(): " << results.back()
                                                << std::endl;
                                        DSOUT() << std::string(depth * 2, ' ') << "r:" << r.elem->id << " [" << r.from_
                                                << "," << r.to_ << "), size=" << results.size() << std::endl;
                                        r.inners = results;
                                        return r;
                                    }
                                }
                            } else {
                                // ???????????????
                                ++el_index;
                                matchers.push_back(Matcher(actual->front_actual(), results.back().to_));
                                DSOUT() << std::string(depth * 2, ' ') << actual->id << ": matchers++ "
                                        << matchers.back().elem->id << std::endl;
                                DSOUT() << std::string(depth * 2, ' ') << elem->id << ":" << depth << ":"
                                        << results.size() << ": [" << results.back().from_ << "," << results.back().to_
                                        << ")" << std::endl;
                                // ??????????????? el_index ?????????????????????????????????
                            }
                        }
                    }
                    case FINISHED:
                        break;
                    default:
                        throw std::runtime_error("unexpected state");
                }

            } else if (actual->is_selection()) {

                if (phase_ == NOT_STARTED) {
                    if (actual->subsidiaries.empty()) {
                        phase_ = FINISHED;
                    } else {
                        phase_   = NTH_RESULT;
                        el_index = 0;
                        matchers.push_back(Matcher(actual->front_actual(), from));
                    }
                }
                switch (phase_) {
                    case NTH_RESULT: {
                        for (; el_index < actual->subsidiaries.size();) {
                            Result r = matchers[0].match(target, depth + 1);
                            if (!r.is_unmatch()) {
                                Result rr(target, actual, r.from_, r.to_);
                                rr.inners.clear();
                                rr.inners.push_back(r);
                                return rr;
                            }
                            ++el_index;
                            if (el_index >= actual->subsidiaries.size()) {
                                break;
                            }
                            matchers[0] = Matcher(actual->nth_actual(el_index), from);
                        }
                        return Result::unmatch(target, actual);
                    }
                    case FINISHED:
                        break;
                    default:
                        throw std::runtime_error("unexpected state");
                }

            } else if (actual->is_concat()) {

                if (phase_ == NOT_STARTED) {
                    if (actual->subsidiaries.empty()) {
                        phase_ = FINISHED;
                    } else {
                        phase_   = NTH_RESULT;
                        el_index = 0;
                        DSOUT() << std::string(depth * 2, ' ') << actual->id << ": matchers++" << std::endl;
                        matchers.push_back(Matcher(actual->nth_actual(el_index), from));
                    }
                }
                switch (phase_) {
                    case NTH_RESULT: {
                        for (; true;) {
                            size_type f;
                            DSOUT() << std::string(depth * 2, ' ') << actual->id << ":el_index: " << el_index
                                    << std::endl;
                            if (el_index == 0) {
                                f = from;
                            } else {
                                f = results.back().to_;
                                DSOUT() << std::string(depth * 2, ' ') << actual->id << ":f <- " << f << " @ "
                                        << results.back().elem->id << std::endl;
                            }
                            results.push_back(matchers[el_index].match(target, depth + 1));
                            if (results.back().is_unmatch()) {
                                if (el_index == 0) {
                                    break;
                                } else {
                                    --el_index;
                                    DSOUT() << std::string(depth * 2, ' ') << actual->id << ": matchers--" << std::endl;
                                    matchers.pop_back();
                                    results.pop_back();
                                }
                            } else {
                                if (el_index == actual->subsidiaries.size() - 1) {
                                    Result r(target, actual, results.front().from_, results.back().to_);
                                    r.inners = results;
                                    DSOUT() << std::string(depth * 2, ' ') << "r: [" << r.from_ << "," << r.to_ << ")"
                                            << std::endl;
                                    return r;
                                } else {
                                    ++el_index;
                                    DSOUT() << std::string(depth * 2, ' ') << actual->id << ": matchers++" << std::endl;
                                    matchers.push_back(Matcher(actual->nth_actual(el_index), results.back().to_));
                                }
                            }
                        }
                        break;
                    }
                    case FINISHED:
                        break;
                    default:
                        throw std::runtime_error("unexpected state");
                }
            }
            return Result::unmatch(target, elem);
        }

        void shift_phase(const Result &result) {
            if (elem->is_terminal()) {
                if (phase_ == NTH_RESULT && tos.empty()) {
                    phase_ = FINISHED;
                }
            } else if (elem->is_repetition()) {
                if (phase_ == NTH_RESULT && matchers.size() == 0) {
                    phase_ = FINISHED;
                }
            } else if (elem->is_selection()) {
                if (phase_ == NTH_RESULT && result.is_unmatch()) {
                    phase_ = FINISHED;
                }
            } else if (elem->is_concat()) {
                if (phase_ == NTH_RESULT && result.is_unmatch()) {
                    phase_ = FINISHED;
                }
            }
        }

        Result match(const string_type &target, size_type depth = 1) {
            Element *actual = elem->actual();
            DSOUT() << std::string((depth - 1) * 2 + 6, ' ') << "[try!] from: " << from << ": for " << actual->id
                    << " phase: " << phase_ << " " << actual->is_terminal() << actual->is_repetition()
                    << actual->is_selection() << actual->is_concat() << std::endl;
            Result r = match_phase(target, from, depth);
            shift_phase(r);
            if (r.is_unmatch()) {
                DSOUT() << std::string((depth - 1) * 2 + 6, ' ') << "[FAIL] from: " << from << ": *fail* for "
                        << elem->id << std::endl;
            } else {
                // DSOUT() << std::string(depth*2, ' ') << "[done] from: " << from << ": matched " << r.from_ << "," <<
                // r.to_ << ") for " << elem->id << ", " << r.inners.size() << std::endl;
                DSOUT() << std::string((depth - 1) * 2 + 6, ' ') << "[done] from: " << from << ": matched " << r
                        << std::endl;
            }
            return r;
        }
    };

    Element def_charset(element_id_type id, const string_type &cs, size_type ge = 1, size_type le = 1) {
        Element el = Element::def_charset(*this, id, cs, ge, le);
        this->register_element(el);
        return el;
    }

    Element
    def_charset(element_id_type id, unsigned char first, unsigned char last, size_type ge = 1, size_type le = 1) {
        Element el = Element::def_charset(*this, id, first, last, ge, le);
        this->register_element(el);
        return el;
    }

    Element def_selection(element_id_type id) {
        Element el = Element::def_selection(*this, id);
        this->register_element(el);
        return el;
    }

    Element def_concat(element_id_type id) {
        Element el = Element::def_concat(*this, id);
        this->register_element(el);
        return el;
    }

    Element def_repetition(element_id_type id, Element &elem, size_type ge, size_type le) {
        Element el = Element::def_repetition(*this, id, ge, le);
        this->register_element(el);
        el.append(elem);
        return el;
    }

    Element def_repetition(element_id_type id, element_id_type eid, size_type ge = 1, size_type le = -1) {
        Element el = Element::def_repetition(*this, id, ge, le);
        this->register_element(el);
        el.append(eid);
        return el;
    }

    void register_element(Element el) {
        if (el.is_surrogate) {
            throw std::runtime_error("cannot register a surrogate element directly");
        }
        element_map.insert(element_map_type::value_type(el.id, el));
        // DSOUT() << "registered: " << el.id << std::endl;
    }

    Result match(element_id_type entry, const string_type &target) {
        element_map_type::iterator it = element_map.find(entry);
        if (it == element_map.end()) {
            throw std::runtime_error("no element for a key");
        }
        for (size_type i = 0; i < target.length(); ++i) {
            Matcher head(&(it->second), i);
            Result r = head.match(target);
            if (!r.is_unmatch()) {
                return r;
            }
        }
        return Result::unmatch(target, &(it->second));
    }
};
} // namespace CFG

int main(int argc, char **argv) {
    const std::string target = argc <= 1 ? "123" : argv[1];
    CFG::Grammar g;
    {
        // scheme
        g.def_charset("alpha", 'a', 'z');                         // alpha = 'a' / 'b' / ... / 'z'
        g.def_charset("Alpha", 'A', 'Z');                         // Alpha = 'A' / 'B' / ... / 'Z'
        g.def_selection("ALPHA").append("alpha").append("Alpha"); // ALPHA = alpha / Alpha
        g.def_charset("DIGIT", '0', '9');
        g.def_selection("ALPHA/DIGIT/+/-/.").append("ALPHA").append("DIGIT").append(g.def_charset("+/-/.", "+-."));
        g.def_repetition("*(ALPHA/DIGIT/+/-/.)", "ALPHA/DIGIT/+/-/.", 0, -1);
        g.def_concat("scheme").append("ALPHA").append("*(ALPHA/DIGIT/+/-/.)");
    }
    {
        // ipv4address
        g.def_concat("ipv4address")
            .append("dec-octet")
            .append(".")
            .append("dec-octet")
            .append(".")
            .append("dec-octet")
            .append(".")
            .append("dec-octet");
        g.def_selection("dec-octet")
            .append("DIGIT")
            .append("10-99")
            .append("100-199")
            .append("200-249")
            .append("250-255");
        g.def_charset(".", ".");
        g.def_charset("2", "2");
        g.def_charset("1-9", '1', '9');
        g.def_charset("0-4", '0', '4');
        g.def_charset("0-5", '0', '5');
        g.def_concat("10-99").append("1-9").append("DIGIT");
        g.def_concat("100-199").append(g.def_charset("1", "1")).append("DIGIT").append("DIGIT");
        g.def_concat("200-249").append("2").append("0-4").append("DIGIT");
        g.def_concat("250-255").append("2").append("0-5").append("0-5");
    }

    {
        g.def_concat("paren word").append("(").append("words").append(")");
        g.def_repetition("words", "word", 0, -1);
        g.def_selection("word").append("paren word").append("char");
        g.def_charset("char", "a", 0, -1);
        g.def_charset("(", "(");
        g.def_charset(")", ")");
    }

    // CFG::Grammar::Result r = g.match("list_of_words", target);
    CFG::Grammar::Result r = g.match("words", target);

    DSOUT() << "\"" << target << "\""
            << " -> "
            << "\"" << r.str() << "\"" << std::endl;
}
