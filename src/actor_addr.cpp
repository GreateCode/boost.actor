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


#include "boost/actor/actor.hpp"
#include "boost/actor/singletons.hpp"
#include "boost/actor/actor_addr.hpp"
#include "boost/actor/local_actor.hpp"

#include "boost/actor/io/middleman.hpp"

#include "boost/actor/detail/raw_access.hpp"

namespace boost {
namespace actor {

namespace {
intptr_t compare_impl(const abstract_actor* lhs, const abstract_actor* rhs) {
    return reinterpret_cast<intptr_t>(lhs) - reinterpret_cast<intptr_t>(rhs);
}
} // namespace <anonymous>

actor_addr::actor_addr(const invalid_actor_addr_t&) : m_ptr(nullptr) { }

actor_addr::actor_addr(abstract_actor* ptr) : m_ptr(ptr) { }

intptr_t actor_addr::compare(const actor_addr& other) const {
    return compare_impl(m_ptr.get(), other.m_ptr.get());
}

intptr_t actor_addr::compare(const abstract_actor* other) const {
    return compare_impl(m_ptr.get(), other);
}

actor_addr actor_addr::operator=(const invalid_actor_addr_t&) {
    m_ptr.reset();
    return *this;
}

actor_id actor_addr::id() const {
    return (m_ptr) ? m_ptr->id() : 0;
}

const node_id& actor_addr::node() const {
    return m_ptr ? m_ptr->node() : *get_middleman()->node();
}

bool actor_addr::is_remote() const {
    return m_ptr ? m_ptr->is_proxy() : false;
}

} // namespace actor
} // namespace boost
