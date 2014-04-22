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


#ifndef BOOST_ACTOR_TO_STRING_HPP
#define BOOST_ACTOR_TO_STRING_HPP

#include <type_traits>

#include "boost/actor/atom.hpp" // included for to_string(atom_value)
#include "boost/actor/actor.hpp"
#include "boost/actor/group.hpp"
#include "boost/actor/object.hpp"
#include "boost/actor/channel.hpp"
#include "boost/actor/node_id.hpp"
#include "boost/actor/anything.hpp"
#include "boost/actor/any_tuple.hpp"
#include "boost/intrusive_ptr.hpp"
#include "boost/actor/abstract_group.hpp"
#include "boost/actor/message_header.hpp"
#include "boost/actor/uniform_type_info.hpp"

namespace std { class exception; }

namespace boost {
namespace actor {

namespace detail {

std::string to_string_impl(const void* what, const uniform_type_info* utype);

template<typename T>
inline std::string to_string_impl(const T& what) {
    return to_string_impl(&what, uniform_typeid<T>());
}

} // namespace detail

inline std::string to_string(const any_tuple& what) {
    return detail::to_string_impl(what);
}

inline std::string to_string(msg_hdr_cref what) {
    return detail::to_string_impl(what);
}

inline std::string to_string(const actor& what) {
    return detail::to_string_impl(what);
}

inline std::string to_string(const actor_addr& what) {
    return detail::to_string_impl(what);
}

inline std::string to_string(const group& what) {
    return detail::to_string_impl(what);
}

inline std::string to_string(const channel& what) {
    return detail::to_string_impl(what);
}

// implemented in node_id.cpp
std::string to_string(const node_id& what);

// implemented in node_id.cpp
std::string to_string(const node_id_ptr& what);

inline std::string to_string(const object& what) {
    return detail::to_string_impl(what.value(), what.type());
}

/**
 * @brief Converts @p e to a string including the demangled type of e
 *        and @p e.what().
 */
std::string to_verbose_string(const std::exception& e);

} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_TO_STRING_HPP
