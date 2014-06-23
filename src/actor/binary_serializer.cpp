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


#include "boost/actor/binary_serializer.hpp"

namespace boost {
namespace actor {

class binary_writer : public static_visitor<> {

 public:

    using write_fun = binary_serializer::write_fun;

    binary_writer(write_fun& sink) : m_out(sink) { }

    template<typename T>
    static inline void write_int(write_fun& f, const T& value) {
        auto first = reinterpret_cast<const char*>(&value);
        auto last = first + sizeof(T);
        f(first, last);
    }

    static inline void write_string(write_fun& f,
                                    const std::string& str) {
        write_int(f, static_cast<uint32_t>(str.size()));
        auto first = str.data();
        auto last = first + str.size();
        f(first, last);
    }

    template<typename T>
    void operator()(const T& value,
                    typename std::enable_if<std::is_integral<T>::value>::type* = 0) const {
        write_int(m_out, value);
    }

    template<typename T>
    void operator()(const T& value,
                    typename std::enable_if<std::is_floating_point<T>::value>::type* = 0) const {
        auto tmp = detail::pack754(value);
        write_int(m_out, tmp);
    }

    // the IEEE-754 conversion does not work for long double
    // => fall back to string serialization (event though it sucks)
    void operator()(const long double& v) const {
        std::ostringstream oss;
        oss << std::setprecision(std::numeric_limits<long double>::digits)
            << v;
        write_string(m_out, oss.str());
    }

    void operator()(const atom_value& val) const {
        (*this)(static_cast<uint64_t>(val));
    }

    void operator()(const std::string& str) const {
        write_string(m_out, str);
    }

    void operator()(const std::u16string& str) const {
        // write size as 32 bit unsigned
        write_int(m_out, static_cast<uint32_t>(str.size()));
        for (char16_t c : str) {
            // force writer to use exactly 16 bit
            write_int(m_out, static_cast<uint16_t>(c));
        }
    }

    void operator()(const std::u32string& str) const {
        // write size as 32 bit unsigned
        write_int(m_out, static_cast<uint32_t>(str.size()));
        for (char32_t c : str) {
            // force writer to use exactly 32 bit
            write_int(m_out, static_cast<uint32_t>(c));
        }
    }

 private:

    write_fun& m_out;

};

void binary_serializer::begin_object(const uniform_type_info* uti) {
    binary_writer::write_string(m_out, uti->name());
}

void binary_serializer::end_object() {
    // NOP
}

void binary_serializer::begin_sequence(size_t list_size) {
    binary_writer::write_int(m_out, static_cast<uint32_t>(list_size));
}

void binary_serializer::end_sequence() {
    // NOP
}

void binary_serializer::write_value(const primitive_variant& value) {
    apply_visitor(binary_writer{m_out}, value);
}

void binary_serializer::write_raw(size_t num_bytes, const void* data) {
    auto first = reinterpret_cast<const char*>(data);
    auto last = first + num_bytes;
    m_out(first, last);
}

} // namespace actor
} // namespace boost
