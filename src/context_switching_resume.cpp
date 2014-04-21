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


#include "boost/actor/policy/context_switching_resume.hpp"
#ifndef BOOST_ACTOR_DISABLE_CONTEXT_SWITCHING

#include <iostream>

#include "boost/actor/cppa.hpp"
#include "boost/actor/exception.hpp"

namespace boost {
namespace actor {
namespace policy {

void context_switching_resume::trampoline(void* this_ptr) {
    BOOST_ACTOR_LOGF_TRACE(BOOST_ACTOR_ARG(this_ptr));
    auto self = reinterpret_cast<blocking_actor*>(this_ptr);
    auto shut_actor_down = [self](std::uint32_t reason) {
        if (self->planned_exit_reason() == exit_reason::not_exited) {
            self->planned_exit_reason(reason);
        }
        self->on_exit();
        self->cleanup(self->planned_exit_reason());
    };
    try {
        self->act();
        shut_actor_down(exit_reason::normal);
    }
    catch (actor_exited& e) {
        shut_actor_down(e.reason());
    }
    catch (...) {
        shut_actor_down(exit_reason::unhandled_exception);
    }
    std::atomic_thread_fence(std::memory_order_seq_cst);
    BOOST_ACTOR_LOGF_DEBUG("done, yield() back to execution unit");;
    detail::yield(detail::yield_state::done);
}

} // namespace policy
} // namespace actor
} // namespace boost

#else // ifdef BOOST_ACTOR_DISABLE_CONTEXT_SWITCHING

namespace boost {
namespace actor {
namespace policy {

void context_switching_resume::trampoline(void*) {
    throw std::logic_error("context_switching_resume::trampoline called");
}

} // namespace policy
} // namespace actor
} // namespace boost

#endif // ifdef BOOST_ACTOR_DISABLE_CONTEXT_SWITCHING
