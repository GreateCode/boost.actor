/******************************************************************************\
 *                                                                            *
 *           ____                  _        _        _                        *
 *          | __ )  ___   ___  ___| |_     / \   ___| |_ ___  _ __            *
 *          |  _ \ / _ \ / _ \/ __| __|   / _ \ / __| __/ _ \| '__|           *
 *          | |_) | (_) | (_) \__ \ |_ _ / ___ \ (__| || (_) | |              *
 *          |____/ \___/ \___/|___/\__(_)_/   \_\___|\__\___/|_|              *
 *                                                                            *
 *                                                                            *
 *                                                                            *
 * Copyright (C) 2011 - 2014                                                  *
 * Dominik Charousset <dominik.charousset (at) haw-hamburg.de>                *
 *                                                                            *
 * Distributed under the Boost Software License, Version 1.0. See             *
 * accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt  *
\******************************************************************************/


#include <stack>
#include <cctype>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <algorithm>

#include "boost/algorithm/string/split.hpp"
#include "boost/algorithm/string/classification.hpp"

#include "boost/actor/atom.hpp"
#include "boost/actor/to_string.hpp"
#include "boost/actor/serializer.hpp"
#include "boost/actor/from_string.hpp"
#include "boost/actor/deserializer.hpp"
#include "boost/actor/skip_message.hpp"
#include "boost/actor/actor_namespace.hpp"
#include "boost/actor/primitive_variant.hpp"
#include "boost/actor/uniform_type_info.hpp"

#include "boost/actor/detail/singletons.hpp"
#include "boost/actor/detail/uniform_type_info_map.hpp"

using std::string;
using std::ostream;
using std::u16string;
using std::u32string;
using std::istringstream;

namespace boost {
namespace actor {

namespace {

bool isbuiltin(const string& type_name) {
    return type_name == "@str" || type_name == "@atom" || type_name == "@tuple";
}

// serializes types as type_name(...) except:
// - strings are serialized "..."
// - atoms are serialized '...'
class string_serializer : public serializer {

    typedef serializer super;

    ostream& out;
    actor_namespace m_namespace;

    struct pt_writer : static_visitor<> {

        ostream& out;
        bool suppress_quotes;

        pt_writer(ostream& mout, bool suppress = false)
            : out(mout), suppress_quotes(suppress) { }

        template<typename T>
        void operator()(const T& value) {
            out << value;
        }

        // make sure char's are treated as int8_t number, not as character
        inline void operator()(const char& value) {
            out << static_cast<int>(value);
        }

        // make sure char's are treated as int8_t number, not as character
        inline void operator()(const unsigned char& value) {
            out << static_cast<unsigned int>(value);
        }

        void operator()(const string& str) {
            if (!suppress_quotes) out << "\"";
            for (char c : str) {
                if (c == '"') out << "\\\"";
                else out << c;
            }
            if (!suppress_quotes) out << "\"";
        }

        void operator()(const u16string&) { }

        void operator()(const u32string&) { }

        void operator()(const atom_value& value) {
            out << "'" << to_string(value) << "'";
        }

    };

    bool m_after_value;
    bool m_obj_just_opened;
    std::stack<size_t> m_object_pos;
    std::stack<string> m_open_objects;

    inline void clear() {
        if (m_after_value) {
            out << ", ";
            m_after_value = false;
        }
        else if (m_obj_just_opened) {
            if (!m_open_objects.empty() && !isbuiltin(m_open_objects.top())) {
                out << " ( ";
            }
            m_obj_just_opened = false;
        }
    }

 public:

    string_serializer(ostream& mout)
    : super(&m_namespace), out(mout), m_after_value(false)
    , m_obj_just_opened(false) { }

    void begin_object(const uniform_type_info* uti) {
        clear();
        string tname = uti->name();
        m_open_objects.push(tname);
        // do not print type names for strings and atoms
        if (!isbuiltin(tname)) out << tname;
        else if (tname.compare(0, 3, "@<>") == 0) {
            std::vector<std::string> subtypes;
            split(subtypes, tname, is_any_of("+"), token_compress_on);

        }
        m_obj_just_opened = true;
    }

    void end_object() {
        if (m_obj_just_opened) {
            // no open brackets to close
            m_obj_just_opened = false;
        }
        m_after_value = true;
        if (!m_open_objects.empty()) {
            if (!isbuiltin(m_open_objects.top())) {
                out << (m_after_value ? " )" : ")");
            }
            m_open_objects.pop();
        }
    }

    void begin_sequence(size_t) {
        clear();
        out << "{ ";
    }

    void end_sequence() {
        out << (m_after_value ? " }" : "}");
        m_after_value = true;
    }

    void write_value(const primitive_variant& value) {
        clear();
        if (m_open_objects.empty()) {
            throw std::runtime_error("write_value(): m_open_objects.empty()");
        }
        pt_writer ptw(out);
        apply_visitor(ptw, value);
        m_after_value = true;
    }

    void write_tuple(size_t size, const primitive_variant* values) {
        clear();
        out << "{";
        const primitive_variant* end = values + size;
        for ( ; values != end; ++values) {
            write_value(*values);
        }
        out << (m_after_value ? " }" : "}");
    }

    void write_raw(size_t num_bytes, const void* buf) {
        clear();
        auto first = reinterpret_cast<const unsigned char*>(buf);
        auto last = first + num_bytes;
        out << std::hex;
        out << std::setfill('0');
        for (; first != last; ++first) {
            out << std::setw(2) << static_cast<size_t>(*first);
        }
        out << std::dec;
        m_after_value = true;
    }

};

class string_deserializer : public deserializer {

    typedef deserializer super;

    typedef string::iterator::difference_type difference_type;

    string m_str;
    string::iterator m_pos;
    //size_t m_obj_count;
    std::stack<bool> m_obj_had_left_parenthesis;
    std::stack<string> m_open_objects;
    actor_namespace m_namespace;

    void skip_space_and_comma() {
        while (*m_pos == ' ' || *m_pos == ',') ++m_pos;
    }

    void throw_malformed [[noreturn]] (const string& error_msg) {
        throw std::logic_error("malformed string: " + error_msg);
    }

    void consume(char c) {
        skip_space_and_comma();
        if (*m_pos != c) {
            string error;
            error += "expected '";
            error += c;
            error += "' found '";
            error += *m_pos;
            error += "'";
            if (m_open_objects.empty() == false) {
                error += "during deserialization an instance of ";
                error += m_open_objects.top();
            }
            throw_malformed(error);
        }
        ++m_pos;
    }

    bool try_consume(char c) {
        skip_space_and_comma();
        if (*m_pos == c) {
            ++m_pos;
            return true;
        }
        return false;
    }

    inline string::iterator next_delimiter() {
        return find_if(m_pos, m_str.end(), [] (char c) -> bool {
            switch (c) {
                case '(':
                case ')':
                case '{':
                case '}':
                case ' ':
                case ',': {
                    return true;
                }
                default: {
                    return false;
                }
            }
        });
    }

    void integrity_check() {
        if (m_open_objects.empty() || m_obj_had_left_parenthesis.empty()) {
            throw_malformed("missing begin_object()");
        }
        if (   m_obj_had_left_parenthesis.top() == false
            && !isbuiltin(m_open_objects.top())) {
            throw_malformed("expected left parenthesis after "
                            "begin_object call or void value");
        }
    }

 public:

    string_deserializer(string str) : super(&m_namespace), m_str(std::move(str)) {
        m_pos = m_str.begin();
    }

    const uniform_type_info* begin_object() override {
        skip_space_and_comma();
        string type_name;
        // shortcuts for builtin types
        if (*m_pos == '"') {
            type_name = "@str";
        }
        else if (*m_pos == '\'') {
            type_name = "@atom";
        }
        else if (*m_pos == '{') {
            type_name = "@tuple";
        }
        else {
            auto substr_end = next_delimiter();
            if (m_pos == substr_end) {
                throw_malformed("could not seek object type name");
            }
            type_name = string(m_pos, substr_end);
            m_pos = substr_end;
        }
        m_open_objects.push(type_name);
        //++m_obj_count;
        skip_space_and_comma();
        // suppress leading parenthesis for builtin types
        m_obj_had_left_parenthesis.push(try_consume('('));
        //consume('(');
        return detail::singletons::get_uniform_type_info_map()
               ->by_uniform_name(type_name);
    }

    void end_object() override {
        if (m_open_objects.empty()) {
            throw std::runtime_error("no object to end");
        }
        if (m_obj_had_left_parenthesis.top() == true) {
            consume(')');
        }
        m_open_objects.pop();
        m_obj_had_left_parenthesis.pop();
        if (m_open_objects.empty()) {
            skip_space_and_comma();
            if (m_pos != m_str.end()) {
                throw_malformed(string("expected end of of string, found: ") + *m_pos);
            }
        }
    }

    size_t begin_sequence() override {
        integrity_check();
        consume('{');
        auto num_vals = count(m_pos, find(m_pos, m_str.end(), '}'), ',') + 1;
        return static_cast<size_t>(num_vals);
    }

    void end_sequence() override {
        integrity_check();
        consume('}');
    }

    struct from_string_reader : static_visitor<> {
        const string& str;
        from_string_reader(const string& s) : str(s) { }
        template<typename T>
        void operator()(T& what) {
            istringstream s(str);
            s >> what;
        }
        void operator()(char& what) {
            istringstream s(str);
            int tmp;
            s >> tmp;
            what = static_cast<char>(tmp);
        }
        void operator()(unsigned char& what) {
            istringstream s(str);
            unsigned int tmp;
            s >> tmp;
            what = static_cast<unsigned char>(tmp);
        }
        void operator()(string& what) {
            what = str;
        }
        void operator()(atom_value& what) {
            what = static_cast<atom_value>(detail::atom_val(str.c_str(), 0xF));
        }
        void operator()(u16string&) {
            throw std::logic_error("u16string currently not supported "
                                   "by string_deserializer");
        }
        void operator()(u32string&) {
            throw std::logic_error("u32string currently not supported "
                                   "by string_deserializer");
        }
    };

    void read_value(primitive_variant& storage) override {
        integrity_check();
        skip_space_and_comma();
        string::iterator substr_end;
        auto find_if_cond = [] (char c) -> bool {
            switch (c) {
             case ')':
             case '}':
             case ' ':
             case ',': return true;
             default : return false;
            }
        };
        if (get<string>(&storage) || get<atom_value>(&storage)) {
            char needle = (get<string>(&storage)) ? '"' : '\'';
            if (*m_pos == needle) {
                // skip leading "
                ++m_pos;
                char last_char = needle;
                auto find_if_str_cond = [&last_char, needle] (char c) -> bool {
                    if (c == needle && last_char != '\\') {
                        return true;
                    }
                    last_char = c;
                    return false;
                };
                substr_end = find_if(m_pos, m_str.end(), find_if_str_cond);
            }
            else {
                substr_end = find_if(m_pos, m_str.end(), find_if_cond);
            }
        }
        else {
            substr_end = find_if(m_pos, m_str.end(), find_if_cond);
        }
        if (substr_end == m_str.end()) {
            throw std::logic_error("malformed string (unterminated value)");
        }
        string substr(m_pos, substr_end);
        m_pos += static_cast<difference_type>(substr.size());
        if (get<string>(&storage) || get<atom_value>(&storage)) {
            char needle = (get<string>(&storage)) ? '"' : '\'';
            // skip trailing "
            if (*m_pos != needle) {
                string error_msg;
                error_msg  = "malformed string, expected '";
                error_msg += needle;
                error_msg += "' found '";
                error_msg += *m_pos;
                error_msg += "'";
                throw std::logic_error(error_msg);
            }
            ++m_pos;
            // replace '\"' by '"'
            char last_char = ' ';
            auto cond = [&last_char, needle] (char c) -> bool {
                if (c == needle && last_char == '\\') {
                    return true;
                }
                last_char = c;
                return false;
            };
            string tmp;
            auto sbegin = substr.begin();
            auto send = substr.end();
            for (auto i = find_if(sbegin, send, cond);
                 i != send;
                 i = find_if(i, send, cond)) {
                --i;
                tmp.append(sbegin, i);
                tmp += needle;
                i += 2;
                sbegin = i;
            }
            if (sbegin != substr.begin()) {
                tmp.append(sbegin, send);
            }
            if (!tmp.empty()) {
                substr = std::move(tmp);
            }
        }
        from_string_reader fsr(substr);
        apply_visitor(fsr, storage);
    }

    void read_raw(size_t buf_size, void* vbuf) override {
        auto buf = reinterpret_cast<unsigned char*>(vbuf);
        integrity_check();
        skip_space_and_comma();
        auto next_nibble = [&]() -> size_t {
            if (*m_pos == '\0') {
                throw_malformed("unexpected end-of-string");
            }
            char c = *m_pos++;
            if (!isxdigit(c)) {
                throw_malformed("unexpected character, expected [0-9a-f]");
            }
            return static_cast<size_t>(isdigit(c) ? c - '0' : (c - 'a' + 10));
        };
        for (size_t i = 0; i < buf_size; ++i) {
            auto nibble = next_nibble();
            *buf++ = static_cast<unsigned char>((nibble << 4) | next_nibble());
        }
    }

};

} // namespace <anonymous>

uniform_value from_string(const string& what) {
    string_deserializer strd(what);
    auto utype = strd.begin_object();
    auto result = utype->deserialize(&strd);
    strd.end_object();
    return result;
    return {};
}

namespace detail {

string to_string_impl(const void *what, const uniform_type_info *utype) {
    std::ostringstream osstr;
    string_serializer strs(osstr);
    strs.begin_object(utype);
    utype->serialize(what, &strs);
    strs.end_object();
    return osstr.str();
}

} // namespace detail

string to_verbose_string(const std::exception& e) {
    std::ostringstream oss;
    oss << detail::demangle(typeid(e)) << ": " << e.what();
    return oss.str();
}

ostream& operator<<(ostream& out, skip_message_t) {
     return out << "skip_message";
}

} // namespace actor
} // namespace boost
