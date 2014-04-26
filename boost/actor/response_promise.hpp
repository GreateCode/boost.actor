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


#ifndef BOOST_ACTOR_RESPONSE_PROMISE_HPP
#define BOOST_ACTOR_RESPONSE_PROMISE_HPP

#include "boost/actor/actor.hpp"
#include "boost/actor/message.hpp"
#include "boost/actor/actor_addr.hpp"
#include "boost/actor/message_id.hpp"

namespace boost {
namespace actor {

/**
 * @brief A response promise can be used to deliver a uniquely identifiable
 *        response message from the server (i.e. receiver of the request)
 *        to the client (i.e. the sender of the request).
 */
class response_promise {

 public:

    response_promise() = default;
    response_promise(response_promise&&) = default;
    response_promise(const response_promise&) = default;
    response_promise& operator=(response_promise&&) = default;
    response_promise& operator=(const response_promise&) = default;

    response_promise(const actor_addr& from,
                     const actor_addr& to,
                     const message_id& response_id);

    /**
     * @brief Queries whether this promise is still valid, i.e., no response
     *        was yet delivered to the client.
     */
    inline explicit operator bool() const {
        // handle is valid if it has a receiver
        return static_cast<bool>(m_to);
    }

    /**
     * @brief Sends @p response_message and invalidates this handle afterwards.
     */
    void deliver(message response_message);

 private:

    actor_addr m_from;
    actor_addr m_to;
    message_id m_id;

};

} // namespace actor
} // namespace boost


#endif // BOOST_ACTOR_RESPONSE_PROMISE_HPP
