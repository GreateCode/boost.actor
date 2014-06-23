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


#include <string>
#include <cstdint>
#include <cstring>
#include <sstream>
#include <iterator>
#include <iostream>
#include <exception>
#include <stdexcept>
#include <type_traits>

#include "boost/actor/binary_deserializer.hpp"

#include "boost/actor/detail/logging.hpp"
#include "boost/actor/detail/ieee_754.hpp"
#include "boost/actor/detail/singletons.hpp"
#include "boost/actor/detail/uniform_type_info_map.hpp"

namespace boost {
namespace actor {

namespace {

typedef const void* pointer;

const char* as_char_pointer(pointer ptr) {
    return reinterpret_cast<const char*>(ptr);
}

pointer advanced(pointer ptr, size_t num_bytes) {
    return reinterpret_cast<const char*>(ptr) + num_bytes;
}

inline void range_check(pointer begin, pointer end, size_t read_size) {
    if (advanced(begin, read_size) > end) {
        BOOST_ACTOR_LOGF(BOOST_ACTOR_ERROR, "range_check failed");
        throw std::out_of_range("binary_deserializer::read_range()");
    }
}

pointer read_range(pointer begin, pointer end, std::string& storage);

template<typename T>
pointer read_range(pointer begin, pointer end, T& storage,
                   typename std::enable_if<std::is_integral<T>::value>::type* = 0) {
    range_check(begin, end, sizeof(T));
    memcpy(&storage, begin, sizeof(T));
    return advanced(begin, sizeof(T));
}

template<typename T>
pointer read_range(pointer begin, pointer end, T& storage,
                   typename std::enable_if<std::is_floating_point<T>::value>::type* = 0) {
    typename detail::ieee_754_trait<T>::packed_type tmp;
    auto result = read_range(begin, end, tmp);
    storage = detail::unpack754(tmp);
    return result;
}

// the IEEE-754 conversion does not work for long double
// => fall back to string serialization (event though it sucks)
pointer read_range(pointer begin, pointer end, long double& storage) {
    std::string tmp;
    auto result = read_range(begin, end, tmp);
    std::istringstream iss{std::move(tmp)};
    iss >> storage;
    return result;
}

pointer read_range(pointer begin, pointer end, std::string& storage) {
    uint32_t str_size;
    begin = read_range(begin, end, str_size);
    range_check(begin, end, str_size);
    storage.clear();
    storage.reserve(str_size);
    pointer cpy_end = advanced(begin, str_size);
    copy(as_char_pointer(begin), as_char_pointer(cpy_end), back_inserter(storage));
    return advanced(begin, str_size);
}

template<typename CharType, typename StringType>
pointer read_unicode_string(pointer begin, pointer end, StringType& str) {
    uint32_t str_size;
    begin = read_range(begin, end, str_size);
    str.reserve(str_size);
    for (size_t i = 0; i < str_size; ++i) {
        CharType c;
        begin = read_range(begin, end, c);
        str += static_cast<typename StringType::value_type>(c);
    }
    return begin;
}

pointer read_range(pointer begin, pointer end, atom_value& storage) {
    uint64_t tmp;
    auto result = read_range(begin, end, tmp);
    storage = static_cast<atom_value>(tmp);
    return result;
}

pointer read_range(pointer begin, pointer end, std::u16string& storage) {
    // char16_t is guaranteed to has *at least* 16 bytes,
    // but not to have *exactly* 16 bytes; thus use uint16_t
    return read_unicode_string<uint16_t>(begin, end, storage);
}

pointer read_range(pointer begin, pointer end, std::u32string& storage) {
    // char32_t is guaranteed to has *at least* 32 bytes,
    // but not to have *exactly* 32 bytes; thus use uint32_t
    return read_unicode_string<uint32_t>(begin, end, storage);
}

struct pt_reader : static_visitor<> {

    pointer begin;
    pointer end;

    pt_reader(pointer bbegin, pointer bend) : begin(bbegin), end(bend) { }

    inline void operator()(none_t&) { }

    template<typename T>
    inline void operator()(T& value) {
        begin = read_range(begin, end, value);
    }

};

} // namespace <anonmyous>

binary_deserializer::binary_deserializer(const void* buf,
                                         size_t buf_size,
                                         actor_namespace* ns)
: super(ns), m_pos(buf), m_end(advanced(buf, buf_size)) { }

binary_deserializer::binary_deserializer(const void* bbegin,
                                         const void* bend,
                                         actor_namespace* ns)
: super(ns), m_pos(bbegin), m_end(bend) { }

const uniform_type_info* binary_deserializer::begin_object() {
    std::string tname;
    m_pos = read_range(m_pos, m_end, tname);
    auto uti = detail::singletons::get_uniform_type_info_map()
               ->by_uniform_name(tname);
    if (!uti) {
        std::string err = "received type name \"";
        err += tname;
        err += "\" but no such type is known";
        throw std::runtime_error(err);
    }
    return uti;
}

void binary_deserializer::end_object() { }

size_t binary_deserializer::begin_sequence() {
    BOOST_ACTOR_LOG_TRACE("");
    static_assert(sizeof(size_t) >= sizeof(uint32_t),
                  "sizeof(size_t) < sizeof(uint32_t)");
    uint32_t result;
    m_pos = read_range(m_pos, m_end, result);
    return static_cast<size_t>(result);
}

void binary_deserializer::end_sequence() { }

void binary_deserializer::read_value(primitive_variant& storage) {
    pt_reader ptr(m_pos, m_end);
    apply_visitor(ptr, storage);
    m_pos = ptr.begin;
}

void binary_deserializer::read_raw(size_t num_bytes, void* storage) {
    range_check(m_pos, m_end, num_bytes);
    memcpy(storage, m_pos, num_bytes);
    m_pos = advanced(m_pos, num_bytes);
}

} // namespace actor
} // namespace boost
