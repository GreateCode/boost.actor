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


#ifndef BOOST_ACTOR_UNIFORM_TYPE_INFO_MAP_HPP
#define BOOST_ACTOR_UNIFORM_TYPE_INFO_MAP_HPP

#include <set>
#include <map>
#include <string>
#include <utility>
#include <type_traits>

#include "boost/actor/cppa_fwd.hpp"

#include "boost/actor/atom.hpp"
#include "boost/actor/unit.hpp"
#include "boost/actor/node_id.hpp"
#include "boost/actor/system_messages.hpp"

#include "boost/actor/util/duration.hpp"
#include "boost/actor/util/type_list.hpp"

#include "boost/actor/detail/singleton_mixin.hpp"

#include "boost/actor/io/buffer.hpp"
#include "boost/actor/io/accept_handle.hpp"
#include "boost/actor/io/connection_handle.hpp"

namespace boost { namespace actor { class uniform_type_info; } }

namespace boost {
namespace actor {
namespace detail {

// ordered according to demangled type name (see uniform_type_info_map.cpp)
using mapped_type_list = util::type_list<
    bool,
    acceptor_closed_msg,
    actor,
    actor_addr,
    message,
    atom_value,
    channel,
    connection_closed_msg,
    down_msg,
    exit_msg,
    group,
    group_down_msg,
    io::accept_handle,
    io::buffer,
    io::connection_handle,
    message_header,
    new_connection_msg,
    new_data_msg,
    sync_exited_msg,
    sync_timeout_msg,
    timeout_msg,
    unit_t,
    util::duration,
    node_id_ptr,
    double,
    float,
    long double,
    std::u16string,
    std::u32string,
    std::string,
    std::u16string,
    std::u32string,
    std::map<std::string, std::string>
>;

using zipped_type_list = util::tl_zip_with_index<mapped_type_list>::type;

// lookup table for built-in types
extern const char* mapped_type_names[][2];

template<typename T>
constexpr const char* mapped_name() {
    return mapped_type_names[util::tl_index_of<zipped_type_list, T>::value][1];
}

const char* mapped_name_by_decorated_name(const char* decorated_name);

std::string mapped_name_by_decorated_name(std::string&& decorated_name);

inline const char* mapped_name_by_decorated_name(const std::string& decorated_name) {
    return mapped_name_by_decorated_name(decorated_name.c_str());
}

// lookup table for integer types
extern const char* mapped_int_names[][2];

template<typename T>
constexpr const char* mapped_int_name() {
    return mapped_int_names[sizeof(T)][std::is_signed<T>::value ? 1 : 0];
}

class uniform_type_info_map_helper;

// note: this class is implemented in uniform_type_info.cpp
class uniform_type_info_map {

    friend class uniform_type_info_map_helper;
    friend class singleton_mixin<uniform_type_info_map>;

 public:

    typedef const uniform_type_info* pointer;

    virtual ~uniform_type_info_map();

    virtual pointer by_uniform_name(const std::string& name) = 0;

    virtual pointer by_rtti(const std::type_info& ti) const = 0;

    virtual std::vector<pointer> get_all() const = 0;

    virtual pointer insert(uniform_type_info_ptr uti) = 0;

    static uniform_type_info_map* create_singleton();

    inline void dispose() { delete this; }

    inline void destroy() { delete this; }

    virtual void initialize() = 0;

};

} // namespace detail
} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_UNIFORM_TYPE_INFO_MAP_HPP
