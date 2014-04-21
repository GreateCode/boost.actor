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


#include "boost/actor/policy.hpp"
#include "boost/actor/singletons.hpp"
#include "boost/actor/scoped_actor.hpp"

#include "boost/actor/detail/proper_actor.hpp"
#include "boost/actor/detail/actor_registry.hpp"

namespace boost {
namespace actor {

namespace {

struct impl : blocking_actor {
    void act() override { }
};

blocking_actor* alloc() {
    using namespace policy;
    return new detail::proper_actor<impl,
                                    policies<no_scheduling,
                                             not_prioritizing,
                                             no_resume,
                                             nestable_invoke>>;
}

} // namespace <anonymous>

void scoped_actor::init(bool hide_actor) {
    m_hidden = hide_actor;
    m_self.reset(alloc());
    if (!m_hidden) {
        get_actor_registry()->inc_running();
        m_prev = BOOST_ACTOR_SET_AID(m_self->id());
    }
}

scoped_actor::scoped_actor() {
    init(false);
}

scoped_actor::scoped_actor(bool hide_actor) {
    init(hide_actor);
}

scoped_actor::~scoped_actor() {
    if (!m_hidden) {
        get_actor_registry()->dec_running();
        BOOST_ACTOR_SET_AID(m_prev);
    }
}

} // namespace actor
} // namespace boost
