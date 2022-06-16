#include <string>
#include <iostream>
#include <vector>
#include <map>
#include <stdexcept>
#include "test_common.hpp"

namespace CFG {
    typedef std::string             string_type;
    typedef std::string             element_id_type;
    typedef string_type::size_type  size_type;
    const size_type npos = string_type::npos;

    class Grammar {
    public:

        class Element {
        public:
            Grammar&                grammar;
            element_id_type         id;
            bool                    charset[256];
            std::vector< element_id_type >  subsidiaries;

            enum Mode {
                CONCAT,
                SELECTION,
                REPETITION
            };

            size_type   le;
            size_type   ge;
            Mode        mode;
            bool        is_surrogate;

            Element(Grammar& grammar, element_id_type id, size_type ge, size_type le, Mode mode, bool is_surrogate):
                grammar(grammar), id(id), le(le), ge(ge), mode(mode), is_surrogate(is_surrogate) {
                }

            Element(const Element& other): grammar(other.grammar) {
                *this = other;
            }

            Element&    operator=(const Element& rhs) {
                // DSOUT() << this << ": " << id << " <- " << rhs.id << std::endl;
                id = rhs.id;
                // DSOUT() << this << ": " << id << std::endl;
                subsidiaries = rhs.subsidiaries;
                le = rhs.le;
                ge = rhs.ge;
                mode = rhs.mode;
                is_surrogate = rhs.is_surrogate;
                memcpy(charset, rhs.charset, sizeof(bool) * 256);
                return *this;
            }

            static Element  def_charset(Grammar& grammar, element_id_type id, const string_type& cs, size_type ge = 1, size_type le = 1) {
                Element el(grammar, id, ge, le, REPETITION, false);
                memset(el.charset, 0, sizeof(bool) * 256);
                for (string_type::size_type i = 0; i < cs.length(); ++i) {
                    el.charset[cs[i] % 256] = true;
                }
                return el;
            }

            static Element  def_charset(Grammar& grammar, element_id_type id, unsigned char first, unsigned char last, size_type ge = 1, size_type le = 1) {
                Element el(grammar, id, ge, le, REPETITION, false);
                memset(el.charset, 0, sizeof(bool) * 256);
                for (; first <= last; ++first) {
                    el.charset[first] = true;
                }
                return el;
            }

            static Element  def_selection(Grammar& grammar, element_id_type id) {
                Element el(grammar, id, 1, 1, SELECTION, false);
                return el;
            }

            static Element  def_concat(Grammar& grammar, element_id_type id) {
                Element el(grammar, id, 1, 1, CONCAT, false);
                return el;
            }

            static Element  def_repetition(Grammar& grammar, element_id_type id, size_type ge, size_type le) {
                Element el(grammar, id, ge, le, REPETITION, false);
                return el;
            }

            // predicate: 終端かどうか
            bool    is_terminal() const {
                return mode == REPETITION && subsidiaries.size() == 0;
            }

            bool    is_repetition() const {
                return mode == REPETITION && subsidiaries.size() != 0;
            }

            bool    is_selection() const {
                return !is_terminal() && !is_repetition() && mode == SELECTION;
            }

            bool    is_concat() const {
                return !is_terminal() && !is_repetition() && mode == CONCAT;
            }

            Element*  actual() {
                element_map_type::iterator it = grammar.element_map.find(id);
                if (it == grammar.element_map.end()) {
                    DSOUT() << "!!" << id << std::endl;
                    throw std::runtime_error("no element for a given id");
                }
                return &it->second;
            }

            Element*  nth_actual(size_type i) {
                element_id_type id = subsidiaries[i];
                element_map_type::iterator it = grammar.element_map.find(id);
                if (it == grammar.element_map.end()) {
                    DSOUT() << "!!" << id << std::endl;
                    throw std::runtime_error("no element for a given id");
                }
                return &it->second;
            }

            Element*  front_actual() {
                return nth_actual(0);
            }

            Element*  back_actual() {
                return nth_actual(subsidiaries.size() - 1);
            }

            Element&    append(Element el) {
                this->actual()->subsidiaries.push_back(el.id);
                return *this;
            }

            Element&    append(element_id_type eid) {
                this->actual()->subsidiaries.push_back(eid);
                return *this;
            }

        };

        typedef std::map<element_id_type, Element>  element_map_type;
        element_map_type    element_map;

        struct Result {
            const string_type&  target;
            Element*            elem;
            size_type           from_;
            size_type           to_;
            std::vector<Result> inners;

            bool    is_unmatch() const {
                return from_ > to_;
            }

            Result(const string_type& target, Element* elem, size_type from, size_type to):
                target(target), elem(elem), from_(from), to_(to) {}

            Result(const Result& other): target(other.target), elem(other.elem) {
                *this = other;
            }

            Result&    operator=(const Result& rhs) {
                elem = rhs.elem;
                from_ = rhs.from_;
                to_ = rhs.to_;
                inners = rhs.inners;
                return *this;
            }

            static Result unmatch(const string_type& target, Element* elem) {
                return Result(target, elem, 1, 0);
            }

            string_type str() const {
                if (is_unmatch()) { return "*unmatched*"; }
                return string_type(target.begin() + from_, target.begin() + to_);
            }
        };

        class Matcher {
        public:
            enum t_phase {
                NOT_STARTED,
                NTH_RESULT,
                FINISHED,
                DUMMY
            };

            Element*                elem;
            t_phase                 phase_;
            size_type               el_index;
            std::vector<size_type>  tos;
            std::vector<Matcher>    matchers;
            std::vector<Result>     results;

            Matcher(Element* from):
                elem(from), phase_(NOT_STARTED) {
                    // DSOUT() << from->id << std::endl;
                }

            Matcher&    operator=(const Matcher& rhs) {
                elem = rhs.elem;
                phase_ = rhs.phase_;
                el_index = rhs.el_index;
                tos = rhs.tos;
                matchers = rhs.matchers;
                return *this;
            }


            Result  match_phase(const string_type& target, size_type from, size_type depth) {
                Element* actual = elem->actual();
                // DSOUT() << actual->id << ": " << elem << " -> " << actual << std::endl;
                // DSOUT() << actual->id << " " << actual->is_terminal() << actual->is_repetition() << actual->is_selection() << actual->is_concat() << std::endl;
                if (actual->is_terminal()) {

                    // DSOUT() << "is_terminal, " << phase_ << std::endl;
                    if (phase_ == NOT_STARTED) {
                        size_type   j = from;
                        size_type count = 0;
                        // DSOUT() << actual->ge << " <= " << count << " <= " << actual->le << std::endl;
                        if (actual->ge <= count && count <= actual->le) {
                            tos.push_back(j);
                        }
                        for (; j < target.length() && count < actual->le; ) {
                            // DSOUT() << actual << ":" << actual->id << " j = " << j << ", '" << target[j] << "' " << actual->charset[(unsigned char)(target[j])] << std::endl;
                            if (!actual->charset[(unsigned char)(target[j])]) { break; }
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
                        // DSOUT() << elem->id << " tos: " << tos.size() << ", phase = " << phase_ << std::endl;
                    }
                    switch (phase_) {
                        case NTH_RESULT: {
                            // DSOUT() << elem->id << ": matched." << std::endl;
                            size_type j = tos.back();
                            tos.pop_back();
                            return Result(target, actual, from, j);
                        }
                        case FINISHED:
                            // DSOUT() << elem->id << ": unmatch!" << std::endl;
                            break;
                        default:
                            throw std::runtime_error("unexpected state");
                    }

                } else if (actual->is_repetition()) {

                    // DSOUT() << "is_repetition subs: " << actual->subsidiaries.size() << ", phase: " << phase_ << std::endl;
                    if (phase_ == NOT_STARTED) {
                        if (actual->subsidiaries.empty()) {
                            phase_ = FINISHED;
                        } else {
                            phase_ = NTH_RESULT;
                            el_index = 0;
                            matchers.push_back(Matcher(actual->front_actual()));
                        }
                    }
                    switch (phase_) {
                        case NTH_RESULT: {
                            for (;true;) {
                                // DSOUT() << elem->id << " phase: " << phase_ << " matchers: " << matchers.size() << std::endl;
                                size_type f = results.empty() ? from : results.back().to_;
                                // DSOUT() << elem->id << " el_index: " << el_index << " matchers.size() = " << matchers.size() << std::endl;
                                Result r = matchers.back().match(target, f, depth + 1);
                                if (r.is_unmatch()) {
                                    // DSOUT() << elem->id << ": unmatch!" << std::endl;
                                    matchers.pop_back();
                                    // DSOUT() << "pop_back: " << el_index << std::endl;
                                    if (el_index == 0) {
                                        results.clear();
                                        if (elem->ge <= el_index && el_index <= elem->le) {
                                            return Result(target, actual->front_actual(), from, from);
                                        }
                                        return Result::unmatch(target, actual->front_actual());
                                    } else {
                                        --el_index;
                                        if (el_index + 1 < results.size()) {
                                            results.pop_back();
                                        }
                                        if (elem->ge <= el_index && el_index <= elem->le) {
                                            Result r(target, actual->front_actual(), from, results.back().to_);
                                            r.inners = results;
                                            return r;
                                        }
                                    }
                                } else {
                                    // DSOUT() << elem->id << ": matched." << std::endl;
                                    ++el_index;
                                    matchers.push_back(Matcher(actual->front_actual()));
                                    // DSOUT() << "push_back" << std::endl;
                                    results.push_back(r);
                                }
                            }
                        }
                        case FINISHED:
                            break;
                        default:
                            throw std::runtime_error("unexpected state");
                    }

                } else if (actual->is_selection()) {

                    // DSOUT() << "is_selection" << std::endl;
                    if (phase_ == NOT_STARTED) {
                        if (actual->subsidiaries.empty()) {
                            phase_ = FINISHED;
                        } else {
                            phase_ = NTH_RESULT;
                            el_index = 0;
                            matchers.push_back(Matcher(actual->front_actual()));
                        }
                    }
                    // DSOUT() << actual->id << " el_index: " << el_index << ", subsidiaries: " << actual->subsidiaries.size() << std::endl;
                    switch (phase_) {
                        case NTH_RESULT: {
                            for (; el_index < actual->subsidiaries.size() ;) {
                                // DSOUT() << "TRY " << matchers[0].elem->id << " " << el_index << std::endl;
                                Result r = matchers[0].match(target, from, depth + 1);
                                // DSOUT() << matchers[0].elem->id << " " << el_index << ": unmatch? " << r.is_unmatch() << std::endl;
                                if (!r.is_unmatch()) { return r; }
                                ++el_index;
                                if (el_index >= actual->subsidiaries.size()) { break; }
                                matchers[0] = Matcher(actual->nth_actual(el_index));
                            }
                            return Result::unmatch(target, actual);
                        }
                        case FINISHED:
                            break;
                        default:
                            throw std::runtime_error("unexpected state");
                    }

                } else if (actual->is_concat()) {

                    // DSOUT() << "is_concat" << std::endl;
                    if (phase_ == NOT_STARTED) {
                        if (actual->subsidiaries.empty()) {
                            phase_ = FINISHED;
                        } else {
                            phase_ = NTH_RESULT;
                            el_index = 0;
                            matchers.push_back(Matcher(actual->nth_actual(el_index)));
                        }
                    }
                    switch (phase_) {
                        case NTH_RESULT: {
                            for (;true;) {
                                size_type f;
                                DSOUT() << "el_index: " << el_index << std::endl;
                                if (el_index == 0) {
                                    f = from;
                                } else {
                                    f = results.back().to_;
                                }
                                results.push_back(matchers[el_index].match(target, f, depth + 1));
                                DSOUT() << "added result: [" << results.back().from_ << "," << results.back().to_ << ")" << std::endl;
                                if (results.back().is_unmatch()) {
                                    if (el_index == 0) {
                                        break;
                                    } else {
                                        --el_index;
                                        matchers.pop_back();
                                        results.pop_back();
                                    }
                                } else {
                                    if (el_index == actual->subsidiaries.size() - 1) {
                                        Result r(target, actual, results.front().from_, results.back().to_);
                                        r.inners = results;
                                        return r;
                                    } else {
                                        ++el_index;
                                        matchers.push_back(Matcher(actual->nth_actual(el_index)));
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

            void    shift_phase(const Result& result) {
                // DSOUT() << elem->id << ": " << phase_ << ": " << result.is_unmatch() << std::endl;
                if (elem->is_terminal()) {
                    if (phase_ == NTH_RESULT && tos.empty()) {
                        phase_ = FINISHED;
                    }
                } else if (elem->is_repetition()) {
                    if (phase_ == NTH_RESULT && (result.is_unmatch() || results.empty())) {
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

            Result  match(const string_type& target, size_type from, size_type depth = 0) {
                DSOUT() << std::string(depth*2, ' ') << "[try!] from: " << from << ": for " << elem->id << std::endl;
                Result r = match_phase(target, from, depth);
                shift_phase(r);
                if (r.is_unmatch()) {
                    DSOUT() << std::string(depth*2, ' ') << "[done] from: " << from << ": *fail* for " << elem->id << std::endl;
                } else {
                    DSOUT() << std::string(depth*2, ' ') << "[done] from: " << from << ": matched [" << r.from_ << "," << r.to_ << ") for " << elem->id << std::endl;
                }
                return r;
            }
        };

        Element def_charset(element_id_type id, const string_type& cs, size_type ge = 1, size_type le = 1) {
            Element el = Element::def_charset(*this, id, cs, ge, le);
            this->register_element(el);
            return el;
            
        }

        Element def_charset(element_id_type id, unsigned char first, unsigned char last, size_type ge = 1, size_type le = 1) {
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

        Element def_repetition(element_id_type id, Element& elem, size_type ge, size_type le) {
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

        void    register_element(Element el) {
            if (el.is_surrogate) {
                throw std::runtime_error("cannot register a surrogate element directly");
            }
            element_map.insert(element_map_type::value_type(el.id, el));
            // DSOUT() << "registered: " << el.id << std::endl;
        }

        Result  match(element_id_type entry, const string_type& target) {
            element_map_type::iterator it = element_map.find(entry);
            if (it == element_map.end()) {
                throw std::runtime_error("no element for a key");
            }
            for (size_type i = 0; i < target.length(); ++i) {
                Matcher head(&(it->second));
                Result r = head.match(target, i);
                if (!r.is_unmatch()) {
                    return r;
                }
            }
            return Result::unmatch(target, &(it->second));
        }
    };
}

int main(int argc, char **argv) {
    const std::string target = argc <= 1 ? "123" : argv[1];
    CFG::Grammar g;
    // g.def_charset("alpha_lower", 'a', 'z');
    // g.def_charset("alpha_upper", 'A', 'Z');
    // g.def_charset("_", "_");

    // g.def_selection("word_head") // A-Za-z_
    //     .append("alpha_lower")
    //     .append("alpha_upper")
    //     .append("_");
    // g.def_charset("digit", '0', '9'); // 0-9

    // g.def_selection("word_body") // A-Za-z0-9_
    //     .append("word_head")
    //     .append("digit");
    // g.def_repetition("word_bodies", "word_body", 0);
    // g.def_concat("word") // (A-Za-z_)(A-Za-z0-9_)*
    //     .append("word_head")
    //     .append("word_bodies");

    // g.def_charset("sp", " \t");
    // g.def_repetition("sps", "sp", 0);
    // g.def_charset("comma", ",");
    // g.def_concat("separator") // [ \t]*,[ \t]*
    //     .append("sps")
    //     .append("comma")
    //     .append("sps");

    // g.def_concat("word_after_head")
    //     .append("separator")
    //     .append(g.def_repetition("word?", "word", 0, 1));
    // g.def_repetition("words_after_head", "word_after_head", 0);

    // g.def_concat("list_of_words")
    //     .append("word")
    //     .append("words_after_head");


    g.def_charset("word", "a", 0, -1);
    // g.def_repetition("word", "az", 0);
    g.def_charset("(", "(");
    g.def_charset(")", ")");
    g.def_selection("word_or_paren_word")
        .append("word")
        .append("paren_word");
    g.def_repetition("wpws", "word_or_paren_word", 0);
    g.def_concat("paren_word")
        .append("(")
        .append("wpws")
        .append(")");

    g.def_concat("simple")
        .append("(")
        .append("word")
        .append(")");


    // CFG::Grammar::Result r = g.match("list_of_words", target);
    CFG::Grammar::Result r = g.match("paren_word", target);

    DSOUT() << "\"" << target << "\"" << " -> " << "\"" << r.str() << "\"" << std::endl;
}
