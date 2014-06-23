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


#ifndef BOOST_ACTOR_BINARY_SERIALIZER_HPP
#define BOOST_ACTOR_BINARY_SERIALIZER_HPP

#include <utility>
#include <sstream>
#include <iomanip>
#include <functional>
#include <type_traits>

#include "boost/actor/serializer.hpp"
#include "boost/actor/primitive_variant.hpp"

#include "boost/actor/detail/ieee_754.hpp"
#include "boost/actor/detail/type_traits.hpp"

namespace boost {
namespace actor {

/**
 * @brief Implements the serializer interface with
 *        a binary serialization protocol.
 * @tparam Buffer A class providing a compatible interface to std::vector<char>.
 */
class binary_serializer : public serializer {

    using super = serializer;

 public:

    using write_fun = std::function<void (const char*, const char*)>;

    /**
     * @brief Creates a binary serializer writing to @p write_buffer.
     * @warning @p write_buffer must be guaranteed to outlive @p this
     */
    template<typename OutIter>
    binary_serializer(OutIter iter, actor_namespace* ns = nullptr) : super(ns) {
        struct fun {
            fun(OutIter pos) : m_pos(pos) { }
            void operator()(const char* first, const char* last) {
                m_pos = std::copy(first, last, m_pos);
            }
            OutIter m_pos;
        };
        m_out = fun{iter};
    }

    void begin_object(const uniform_type_info* uti) override;

    void end_object() override;

    void begin_sequence(size_t list_size) override;

    void end_sequence() override;

    void write_value(const primitive_variant& value) override;

    void write_raw(size_t num_bytes, const void* data) override;

 private:

    write_fun m_out;

};

template<typename T,
         class = typename std::enable_if<detail::is_primitive<T>::value>::type>
binary_serializer& operator<<(binary_serializer& bs, const T& value) {
    bs.write_value(value);
    return bs;
}

} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_BINARY_SERIALIZER_HPP
