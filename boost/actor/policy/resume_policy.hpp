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


#ifndef BOOST_ACTOR_RESUME_POLICY_HPP
#define BOOST_ACTOR_RESUME_POLICY_HPP

#include "boost/actor/resumable.hpp"

// this header consists all type definitions needed to
// implement the resume_policy trait

namespace boost {
namespace actor {

class execution_unit;
namespace util { class duration; }
namespace detail { struct cs_thread; }

namespace policy {

/**
 * @brief The resume_policy <b>concept</b> class. Please note that this
 *        class is <b>not</b> implemented. It only explains the all
 *        required member function and their behavior for any resume policy.
 */
class resume_policy {

 public:

    /**
     * @brief Resumes the actor by reading a new message <tt>msg</tt> and then
     *        calling <tt>self->invoke(msg)</tt>. This process is repeated
     *        until either no message is left in the actor's mailbox or the
     *        actor finishes execution.
     */
    template<class Actor>
    resumable::resume_result resume(Actor* self,
                                    detail::cs_thread* from,
                                    execution_unit*);

    /**
     * @brief Waits unconditionally until the actor is ready to resume.
     * @note This member function must raise a compiler error if the resume
     *       strategy cannot be used to implement blocking actors.
     *
     * This member function calls {@link scheduling_policy::await_data}
     */
    template<class Actor>
    bool await_ready(Actor* self);

};

} // namespace policy
} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_RESUME_POLICY_HPP
