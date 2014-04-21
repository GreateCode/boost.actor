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


#include <utility>

#include "boost/actor/actor.hpp"
#include "boost/actor/channel.hpp"
#include "boost/actor/actor_addr.hpp"
#include "boost/actor/actor_proxy.hpp"
#include "boost/actor/local_actor.hpp"
#include "boost/actor/blocking_actor.hpp"
#include "boost/actor/event_based_actor.hpp"

namespace boost {
namespace actor {

actor::actor(const invalid_actor_t&) : m_ptr(nullptr) { }

actor::actor(abstract_actor* ptr) : m_ptr(ptr) { }

actor& actor::operator=(const invalid_actor_t&) {
    m_ptr.reset();
    return *this;
}

intptr_t actor::compare(const actor& other) const {
    return channel::compare(m_ptr.get(), other.m_ptr.get());
}

intptr_t actor::compare(const actor_addr& other) const {
    return m_ptr.compare(other.m_ptr);
}

void actor::swap(actor& other) {
    m_ptr.swap(other.m_ptr);
}

actor_addr actor::address() const {
    return m_ptr ? m_ptr->address() : actor_addr{};
}

} // namespace actor
} // namespace boost
