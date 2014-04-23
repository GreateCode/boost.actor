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


#ifndef BOOST_ACTOR_THREADED_HPP
#define BOOST_ACTOR_THREADED_HPP

#include <mutex>
#include <chrono>
#include <condition_variable>

#include "boost/actor/exit_reason.hpp"
#include "boost/actor/mailbox_element.hpp"

#include "boost/actor/detail/sync_request_bouncer.hpp"

#include "boost/actor/intrusive/single_reader_queue.hpp"

#include "boost/actor/policy/invoke_policy.hpp"

namespace boost { namespace actor {
namespace detail { class receive_policy; } } }

namespace boost {
namespace actor {
namespace policy {

class nestable_invoke : public invoke_policy<nestable_invoke> {

 public:

    inline bool hm_should_skip(mailbox_element* node) {
        return node->marked;
    }

    template<class Actor>
    inline mailbox_element* hm_begin(Actor* self, mailbox_element* node) {
        auto previous = self->current_node();
        self->current_node(node);
        self->push_timeout();
        node->marked = true;
        return previous;
    }

    template<class Actor>
    inline void hm_cleanup(Actor* self, mailbox_element* previous) {
        self->current_node()->marked = false;
        self->current_node(previous);
    }

    template<class Actor>
    inline void hm_revert(Actor* self, mailbox_element* previous) {
        self->current_node()->marked = false;
        self->current_node(previous);
        self->pop_timeout();
    }

};

} // namespace policy
} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_THREADED_HPP
