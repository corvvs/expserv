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
            std::vector< Element >  subsidiary;

            size_type    le;
            size_type    ge;
            bool            as_concat;
            bool            is_surrogate;

            Element(Grammar& grammar, element_id_type id, size_type ge, size_type le, bool as_concat, bool is_surrogate):
                grammar(grammar), id(id), le(le), ge(ge), as_concat(as_concat), is_surrogate(is_surrogate) {}

            Element(const Element& other): grammar(other.grammar) {
                *this = other;
            }

            Element&    operator=(const Element& rhs) {
                id = rhs.id;
                subsidiary = rhs.subsidiary;
                le = rhs.le;
                ge = rhs.ge;
                as_concat = rhs.as_concat;
                is_surrogate = rhs.is_surrogate;
                memcpy(charset, rhs.charset, sizeof(bool) * 256);
                return *this;
            }

            void    set_charset(string_type& cs) {
                memset(charset, 0, sizeof(bool) * 256);
                for (string_type::size_type i = 0; i < cs.length(); ++i) {
                    charset[cs[i] % 256] = true;
                }
            }

            void    set_charset(unsigned char first, unsigned char last) {
                memset(charset, 0, sizeof(bool) * 256);
                if (first > last) { return ; }
                DOUT();
                for (; first <= last; ++first) {
                    charset[first] = true;
                    std::cout << first;
                }
                std::cout << std::endl;
            }

            // predicate: 終端かどうか
            bool    is_terminal() const {
                return subsidiary.size() == 0;
            }

            bool    is_selection() const {
                return !is_terminal() && !as_concat;
            }

            bool    is_concat() const {
                return !is_terminal() && as_concat;
            }

            Element*  actual() {
                if (is_surrogate) {
                    element_map_type::iterator it = grammar.element_map.find(id);
                    return &(it->second);
                } else {
                    return this;
                }
            }

            void    append(Element& el) {
                subsidiary.push_back(el);
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
            size_type            el_index;
            std::vector<size_type>  tos;
            std::vector<Matcher>    matchers;
            std::vector<Result>     results;

            Matcher(Element* from):
                elem(from), phase_(NOT_STARTED) {}

            Matcher&    operator=(const Matcher& rhs) {
                elem = rhs.elem;
                phase_ = rhs.phase_;
                el_index = rhs.el_index;
                tos = rhs.tos;
                matchers = rhs.matchers;
                return *this;
            }


            Result  match_phase(const string_type& target, size_type from) {
                Element* actual = elem->actual();
                DOUT() << elem->id << " " << actual->is_terminal() << actual->is_selection() << actual->is_concat() << std::endl;
                if (actual->is_terminal()) {
                    DOUT() << "is_terminal, " << phase_ << std::endl;
                    if (phase_ == NOT_STARTED) {
                        size_type   j = from;
                        size_type count = 0;
                        DOUT() << actual->ge << " <= " << count << " <= " << actual->le << std::endl;
                        if (actual->ge <= count && count <= actual->le) {
                            tos.push_back(j);
                        }
                        for (; j < target.length() && count < actual->le; ) {
                            DOUT() << elem->id << " j = " << j << ", '" << target[j] << "' " << actual->charset[(unsigned char)(target[j])] << std::endl;
                            DOUT() << "[";
                            for(unsigned int c = 0; c <= 255; ++c) {
                                if (actual->charset[c]) {
                                    std::cout << (unsigned char)c;
                                }
                            }
                            std::cout << "]" << std::endl;
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
                        DOUT() << "tos: " << tos.size() << ", phase = " << phase_ << std::endl;
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
                } else if (actual->is_selection()) {
                    DOUT() << "is_selection" << std::endl;
                    if (phase_ == NOT_STARTED) {
                        if (actual->subsidiary.empty()) {
                            phase_ = FINISHED;
                        } else {
                            phase_ = NTH_RESULT;
                            el_index = 0;
                            matchers.push_back(Matcher(&(actual->subsidiary[0])));
                        }
                    }
                    DOUT() << "el_index: " << el_index << ", subsidiary: " << actual->subsidiary.size() << std::endl;
                    switch (phase_) {
                        case NTH_RESULT: {
                            for (int i = 0; i <actual->subsidiary.size(); ++i) {
                                DOUT() << i << ": " << actual->subsidiary[i].id << std::endl;
                            }
                            for (; el_index < actual->subsidiary.size() ;) {
                                DOUT() << "TRY " << matchers[0].elem->id << " " << el_index << std::endl;
                                Result r = matchers[0].match(target, from);
                                DOUT() << matchers[0].elem->id << " " << el_index << ": unmatch? " << r.is_unmatch() << std::endl;
                                if (!r.is_unmatch()) { return r; }
                                ++el_index;
                                if (el_index >= actual->subsidiary.size()) { break; }
                                matchers[0] = Matcher(&(actual->subsidiary[el_index]));
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
                        if (actual->subsidiary.empty()) {
                            phase_ = FINISHED;
                        } else {
                            phase_ = NTH_RESULT;
                            el_index = 0;
                            matchers.push_back(Matcher(&(actual->subsidiary[el_index])));
                        }
                    }
                    switch (phase_) {
                        case NTH_RESULT: {
                            for (;true;) {
                                size_type f;
                                if (el_index == 0) {
                                    f = from;
                                } else {
                                    f = results[el_index - 1].to_;
                                }
                                results.push_back(matchers[el_index].match(target, f));
                                if (results.back().is_unmatch()) {
                                    if (el_index == 0) {
                                        break;
                                    } else {
                                        --el_index;
                                        matchers.pop_back();
                                        results.pop_back();
                                    }
                                } else {
                                    if (el_index == actual->subsidiary.size() - 1) {
                                        Result r(target, actual, results.front().from_, results.back().to_);
                                        r.inners = results;
                                        return r;
                                    } else {
                                        ++el_index;
                                        matchers.push_back(Matcher(&actual->subsidiary[el_index]));
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
                if (elem->is_terminal()) {
                    if (phase_ == NTH_RESULT && tos.empty()) {
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


            Result  match(const string_type& target, size_type from) {
                Result r = match_phase(target, from);
                shift_phase(r);
                return r;
            }
        };

        void    register_element(Element& el) {
            if (el.is_surrogate) {
                throw std::runtime_error("cannot register a surrogate element directly");
            }
            element_map.insert(element_map_type::value_type(el.id, el));
        }

        Result  match(element_id_type entry, const string_type& target) {
            element_map_type::iterator it = element_map.find(entry);
            if (it == element_map.end()) {
                throw std::runtime_error("no element for a key");
            }
            for (size_type i = 0; i < target.length(); ++i) {
                Matcher head(&(it->second));
                DOUT() << "i = " << i << std::endl;
                Result r = head.match(target, i);
                if (!r.is_unmatch()) {
                    return r;
                }
            }
            return Result::unmatch(target, &(it->second));
        }
    };
}

int main() {
    std::string target = "123";
    CFG::Grammar g;
    CFG::Grammar::Element   alpha_lower(g, "alpha_lower", 1, 1, false, false);
    alpha_lower.set_charset('a', 'z');
    CFG::Grammar::Element   alpha_upper(g, "alpha_upper", 1, 1, false, false);
    alpha_upper.set_charset('A', 'Z');
    CFG::Grammar::Element   digit(g, "digit", 1, 1, false, false);
    digit.set_charset('0', '9');
    CFG::Grammar::Element   alphanumeric(g, "alphanumeric", 1, 1, false, false);
    alphanumeric.append(alpha_lower);
    alphanumeric.append(alpha_upper);
    alphanumeric.append(digit);
    CFG::Grammar::Element   alphanumerics(g, "alphanumerics", 0, -1, false, false);
    alphanumerics.append(alphanumeric);
    g.register_element(alpha_lower);
    g.register_element(alpha_upper);
    g.register_element(digit);
    g.register_element(alphanumeric);
    g.register_element(alphanumerics);
    CFG::Grammar::Result r = g.match("alphanumerics", target);
    DOUT() << target << " -> " << r.str() << std::endl;
}

