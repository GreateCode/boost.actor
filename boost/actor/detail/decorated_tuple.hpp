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


#ifndef BOOST_ACTOR_DECORATED_TUPLE_HPP
#define BOOST_ACTOR_DECORATED_TUPLE_HPP

#include <vector>
#include <algorithm>

#include "boost/actor/config.hpp"
#include "boost/actor/ref_counted.hpp"
#include "boost/actor/uniform_type_info.hpp"

#include "boost/actor/util/type_list.hpp"

#include "boost/actor/detail/tuple_vals.hpp"
#include "boost/actor/detail/message_data.hpp"
#include "boost/actor/detail/serialize_tuple.hpp"

namespace boost {
namespace actor {
namespace detail {

class decorated_tuple : public message_data {

    typedef message_data super;

    decorated_tuple& operator=(const decorated_tuple&) = delete;

 public:

    typedef std::vector<size_t> vector_type;

    typedef message_data::ptr pointer;

    typedef const std::type_info* rtti;

    // creates a dynamically typed subtuple from @p d with an offset
    static inline pointer create(pointer d, vector_type v) {
        return pointer{new decorated_tuple(std::move(d), std::move(v))};
    }

    // creates a statically typed subtuple from @p d with an offset
    static inline pointer create(pointer d, rtti ti, vector_type v) {
        return pointer{new decorated_tuple(std::move(d), ti, std::move(v))};
    }

    // creates a dynamically typed subtuple from @p d with an offset
    static inline pointer create(pointer d, size_t offset) {
        return pointer{new decorated_tuple(std::move(d), offset)};
    }

    // creates a statically typed subtuple from @p d with an offset
    static inline pointer create(pointer d, rtti ti, size_t offset) {
        return pointer{new decorated_tuple(std::move(d), ti, offset)};
    }

    void* mutable_at(size_t pos) override;

    size_t size() const override;

    decorated_tuple* copy() const override;

    const void* at(size_t pos) const override;

    const uniform_type_info* type_at(size_t pos) const override;

    const std::string* tuple_type_names() const override;

    rtti type_token() const override;

 private:

    pointer     m_decorated;
    rtti        m_token;
    vector_type m_mapping;

    void init();

    void init(size_t);

    decorated_tuple(pointer, size_t);

    decorated_tuple(pointer, rtti, size_t);

    decorated_tuple(pointer, vector_type&&);

    decorated_tuple(pointer, rtti, vector_type&&);

    decorated_tuple(const decorated_tuple&) = default;

};

} // namespace detail
} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_DECORATED_TUPLE_HPP
