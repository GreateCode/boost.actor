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


#ifndef BOOST_ACTOR_ACTOR_PROXY_HPP
#define BOOST_ACTOR_ACTOR_PROXY_HPP

#include "boost/actor/extend.hpp"
#include "boost/actor/abstract_actor.hpp"
#include "boost/actor/abstract_actor.hpp"
#include "boost/actor/message_header.hpp"

namespace boost {
namespace actor {

class actor_proxy_cache;

/**
 * @brief Represents a remote actor.
 * @extends abstract_actor
 */
class actor_proxy : public abstract_actor {

    typedef abstract_actor super;

 public:

    ~actor_proxy();

    /**
     * @brief Establishes a local link state that's not synchronized back
     *        to the remote instance.
     */
    virtual void local_link_to(const actor_addr& other) = 0;

    /**
     * @brief Removes a local link state.
     */
    virtual void local_unlink_from(const actor_addr& other) = 0;

    /**
     * @brief Delivers given message via this proxy instance.
     *
     * This function is meant to give the proxy the opportunity to keep track
     * of synchronous communication or perform other bookkeeping if needed.
     * The member function is called by the protocol from inside the
     * middleman's thread.
     * @note This function is guaranteed to be called non-concurrently.
     */
    virtual void deliver(msg_hdr_cref hdr, any_tuple msg) = 0;

 protected:

    actor_proxy(actor_id mid);

};

/**
 * @brief A smart pointer to an {@link actor_proxy} instance.
 * @relates actor_proxy
 */
typedef intrusive_ptr<actor_proxy> actor_proxy_ptr;

} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_ACTOR_PROXY_HPP
