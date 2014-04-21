/******************************************************************************\
 *           ___        __                                                    *
 *          /\_ \    __/\ \                                                   *
 *          \//\ \  /\_\ \ \____    ___   _____   _____      __               *
 *            \ \ \ \/\ \ \ '__`\  /'___\/\ '__`\/\ '__`\  /'__`\             *
 *             \_\ \_\ \ \ \ \L\ \/\ \__/\ \ \L\ \ \ \L\ \/\ \L\.\_           *
 *             /\____\\ \_\ \_,__/\ \____\\ \ ,__/\ \ ,__/\ \__/.\_\          *
 *             \/____/ \/_/\/___/  \/____/ \ \ \/  \ \ \/  \/__/\/_/          *
 *                                          \ \_\   \ \_\                     *
 *                                           \/_/    \/_/                     *
 *                                                                            *
 * Copyright (C) 2011-2013                                                    *
 * Dominik Charousset <dominik.charousset@haw-hamburg.de>                     *
 *                                                                            *
 * This file is part of libcppa.                                              *
 * libcppa is free software: you can redistribute it and/or modify it under   *
 * the terms of the GNU Lesser General Public License as published by the     *
 * Free Software Foundation; either version 2.1 of the License,               *
 * or (at your option) any later version.                                     *
 *                                                                            *
 * libcppa is distributed in the hope that it will be useful,                 *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                       *
 * See the GNU Lesser General Public License for more details.                *
 *                                                                            *
 * You should have received a copy of the GNU Lesser General Public License   *
 * along with libcppa. If not, see <http://www.gnu.org/licenses/>.            *
\******************************************************************************/


#ifndef BOOST_ACTOR_DESERIALIZER_HPP
#define BOOST_ACTOR_DESERIALIZER_HPP

#include <string>
#include <cstddef>

#include "boost/actor/primitive_type.hpp"
#include "boost/actor/primitive_variant.hpp"

namespace boost {
namespace actor {

class object;
class actor_namespace;
class type_lookup_table;

namespace util { class buffer; }

/**
 * @ingroup TypeSystem
 * @brief Technology-independent deserialization interface.
 */
class deserializer {

    deserializer(const deserializer&) = delete;
    deserializer& operator=(const deserializer&) = delete;

 public:

    deserializer(actor_namespace* ns = nullptr,
                 type_lookup_table* incoming_types = nullptr);

    virtual ~deserializer();

    /**
     * @brief Seeks the beginning of the next object and return
     *        its uniform type name.
     */
    //virtual std::string seek_object() = 0;

    /**
     * @brief Begins deserialization of a new object.
     */
    virtual const uniform_type_info* begin_object() = 0;

    /**
     * @brief Ends deserialization of an object.
     */
    virtual void end_object() = 0;

    /**
     * @brief Begins deserialization of a sequence.
     * @returns The size of the sequence.
     */
    virtual size_t begin_sequence() = 0;

    /**
     * @brief Ends deserialization of a sequence.
     */
    virtual void end_sequence() = 0;

    /**
     * @brief Reads a primitive value from the data source of type @p ptype.
     * @param ptype Expected primitive data type.
     * @returns A primitive value of type @p ptype.
     */
    virtual primitive_variant read_value(primitive_type ptype) = 0;

    /**
     * @brief Reads a value of type @p T from the data source of type @p ptype.
     * @note @p T must be of a primitive type.
     * @returns The read value of type @p T.
     */
    template<typename T>
    inline T read() {
        auto val = read_value(detail::type_to_ptype<T>::ptype);
        return std::move(get_ref<T>(val));
    }

    /**
     * @brief Reads a tuple of primitive values from the data
     *        source of the types @p ptypes.
     * @param num The size of the tuple.
     * @param ptypes Array of expected primitive data types.
     * @param storage Array of size @p num, storing the result of this function.
     */
    virtual void read_tuple(size_t num,
                            const primitive_type* ptypes,
                            primitive_variant* storage   ) = 0;

    /**
     * @brief Reads a raw memory block.
     */
    virtual void read_raw(size_t num_bytes, void* storage) = 0;

    inline actor_namespace* get_namespace() {
        return m_namespace;
    }

    inline type_lookup_table* incoming_types() {
        return m_incoming_types;
    }

    void read_raw(size_t num_bytes, util::buffer& storage);

 private:

    actor_namespace* m_namespace;
    type_lookup_table* m_incoming_types;

};

} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_DESERIALIZER_HPP
