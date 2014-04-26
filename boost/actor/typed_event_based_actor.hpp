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


#ifndef BOOST_ACTOR_TYPED_EVENT_BASED_ACTOR_HPP
#define BOOST_ACTOR_TYPED_EVENT_BASED_ACTOR_HPP

#include "boost/actor/replies_to.hpp"
#include "boost/actor/local_actor.hpp"
#include "boost/actor/typed_behavior.hpp"

#include "boost/actor/mixin/sync_sender.hpp"
#include "boost/actor/mixin/mailbox_based.hpp"
#include "boost/actor/mixin/behavior_stack_based.hpp"

namespace boost {
namespace actor {

/**
 * @brief A cooperatively scheduled, event-based actor implementation
 *        with strong type checking.
 *
 * This is the recommended base class for user-defined actors and is used
 * implicitly when spawning typed, functor-based actors without the
 * {@link blocking_api} flag.
 *
 * @extends local_actor
 */
template<typename... Rs>
class typed_event_based_actor
        : public extend<local_actor, typed_event_based_actor<Rs...>>::template
                 with<mixin::mailbox_based,
                      mixin::behavior_stack_based<
                          typed_behavior<Rs...>
                      >::template impl,
                      mixin::sync_sender<
                          nonblocking_response_handle_tag
                      >::template impl> {

 public:

    typed_event_based_actor() : m_initialized(false) { }

    typedef detail::type_list<Rs...> signatures;

    typedef typed_behavior<Rs...> behavior_type;

    std::set<std::string> interface() const override {
        return {detail::to_uniform_name<Rs>()...};
    }

 protected:

    virtual behavior_type make_behavior() = 0;

    bool m_initialized;

};

} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_TYPED_EVENT_BASED_ACTOR_HPP
