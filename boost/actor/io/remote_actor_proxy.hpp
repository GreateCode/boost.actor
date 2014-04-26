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


#ifndef REMOTE_ACTOR_PROXY_HPP
#define REMOTE_ACTOR_PROXY_HPP

#include "boost/actor/extend.hpp"
#include "boost/actor/actor_proxy.hpp"
#include "boost/actor/memory_cached.hpp"

#include "boost/actor/detail/single_reader_queue.hpp"

namespace boost {
namespace actor {
namespace detail {

class memory;
class instance_wrapper;
template<typename>
class basic_memory_cache;

} } // namespace actor
} // namespace boost::detail

namespace boost {
namespace actor {
namespace io {

class middleman;

class sync_request_info : public extend<memory_managed>::with<memory_cached> {

    friend class detail::memory;

 public:

    typedef sync_request_info* pointer;

    ~sync_request_info();

    pointer    next;   // intrusive next pointer
    actor_addr sender; // points to the sender of the message
    message_id mid;    // sync message ID

 private:

    sync_request_info(actor_addr sptr, message_id id);

};

class remote_actor_proxy : public actor_proxy {

    typedef actor_proxy super;

 public:

    remote_actor_proxy(actor_id mid,
                       node_id_ptr pinfo,
                       middleman* parent);

    void enqueue(msg_hdr_cref hdr, message msg, execution_unit*) override;

    void link_to(const actor_addr& other) override;

    void unlink_from(const actor_addr& other) override;

    bool remove_backlink(const actor_addr& to) override;

    bool establish_backlink(const actor_addr& to) override;

    void local_link_to(const actor_addr& other) override;

    void local_unlink_from(const actor_addr& other) override;

    void deliver(msg_hdr_cref hdr, message msg) override;

 protected:

    ~remote_actor_proxy();

 private:

    void forward_msg(msg_hdr_cref hdr, message msg);

    middleman* m_parent;
    detail::single_reader_queue<sync_request_info, detail::disposer> m_pending_requests;

};

} // namespace io
} // namespace actor
} // namespace boost

#endif // remote_actor_proxy_HPP
