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


#include <string>
#include "boost/actor/cppa.hpp"
#include "boost/actor/atom.hpp"
#include "boost/actor/logging.hpp"
#include "boost/actor/scheduler.hpp"
#include "boost/actor/local_actor.hpp"

#include "boost/actor/detail/raw_access.hpp"

namespace boost {
namespace actor {

namespace {

class down_observer : public attachable {

    actor_addr m_observer;
    actor_addr m_observed;

 public:

    down_observer(actor_addr observer, actor_addr observed)
    : m_observer(std::move(observer)), m_observed(std::move(observed)) {
        BOOST_ACTOR_REQUIRE(m_observer != nullptr);
        BOOST_ACTOR_REQUIRE(m_observed != nullptr);
    }

    void actor_exited(std::uint32_t reason) {
        auto ptr = detail::raw_access::get(m_observer);
        message_header hdr{m_observed, ptr, message_id{}.with_high_priority()};
        hdr.deliver(make_message(down_msg{m_observed, reason}));
    }

    bool matches(const attachable::token& match_token) {
        if (match_token.subtype == typeid(down_observer)) {
            auto ptr = reinterpret_cast<const local_actor*>(match_token.ptr);
            return m_observer == ptr->address();
        }
        return false;
    }

};

} // namespace <anonymous>

local_actor::local_actor()
        : m_trap_exit(false), m_dummy_node(), m_current_node(&m_dummy_node)
        , m_planned_exit_reason(exit_reason::not_exited) {
    m_node = get_middleman()->node();
}

local_actor::~local_actor() { }

void local_actor::monitor(const actor_addr& whom) {
    if (!whom) return;
    auto ptr = detail::raw_access::get(whom);
    ptr->attach(attachable_ptr{new down_observer(address(), whom)});
}

void local_actor::demonitor(const actor_addr& whom) {
    if (!whom) return;
    auto ptr = detail::raw_access::get(whom);
    attachable::token mtoken{typeid(down_observer), this};
    ptr->detach(mtoken);
}

void local_actor::on_exit() { }

void local_actor::join(const group& what) {
    BOOST_ACTOR_LOG_TRACE(BOOST_ACTOR_TSARG(what));
    if (what && m_subscriptions.count(what) == 0) {
        BOOST_ACTOR_LOG_DEBUG("join group: " << to_string(what));
        m_subscriptions.insert(std::make_pair(what, what->subscribe(this)));
    }
}

void local_actor::leave(const group& what) {
    if (what) m_subscriptions.erase(what);
}

std::vector<group> local_actor::joined_groups() const {
    std::vector<group> result;
    for (auto& kvp : m_subscriptions) {
        result.emplace_back(kvp.first);
    }
    return result;
}

void local_actor::reply_message(message&& what) {
    auto& whom = m_current_node->sender;
    if (!whom) return;
    auto& id = m_current_node->mid;
    if (id.valid() == false || id.is_response()) {
        send_tuple(detail::raw_access::get(whom), std::move(what));
    }
    else if (!id.is_answered()) {
        auto ptr = detail::raw_access::get(whom);
        ptr->enqueue({address(), ptr, id.response_id()},
                     std::move(what), m_host);
        id.mark_as_answered();
    }
}

void local_actor::forward_message(const actor& dest, message_priority prio) {
    if (!dest) return;
    auto id = (prio == message_priority::high)
            ? m_current_node->mid.with_high_priority()
            : m_current_node->mid.with_normal_priority();
    auto p = detail::raw_access::get(dest);
    p->enqueue({m_current_node->sender, p, id}, m_current_node->msg, m_host);
    // treat this message as asynchronous message from now on
    m_current_node->mid = message_id::invalid;
}

void local_actor::send_tuple(message_priority prio, const channel& dest, message what) {
    if (!dest) return;
    message_id id;
    if (prio == message_priority::high) id = id.with_high_priority();
    dest->enqueue({address(), dest, id}, std::move(what), m_host);
}

void local_actor::send_exit(const actor_addr& whom, std::uint32_t reason) {
    send(detail::raw_access::get(whom), exit_msg{address(), reason});
}

void local_actor::delayed_send_tuple(message_priority prio,
                                     const channel& dest,
                                     const util::duration& rel_time,
                                     message msg) {
    message_id mid;
    if (prio == message_priority::high) mid = mid.with_high_priority();
    get_scheduling_coordinator()->delayed_send({address(), dest, mid},
                                  rel_time, std::move(msg));
}

response_promise local_actor::make_response_promise() {
    auto n = m_current_node;
    response_promise result{address(), n->sender, n->mid.response_id()};
    n->mid.mark_as_answered();
    return result;
}

void local_actor::cleanup(std::uint32_t reason) {
    BOOST_ACTOR_LOG_TRACE(BOOST_ACTOR_ARG(reason));
    m_subscriptions.clear();
    super::cleanup(reason);
}

void local_actor::quit(std::uint32_t reason) {
    BOOST_ACTOR_LOG_TRACE("reason = " << reason
                   << ", class " << detail::demangle(typeid(*this)));
    if (reason == exit_reason::unallowed_function_call) {
        // this is the only reason that causes an exception
        cleanup(reason);
        BOOST_ACTOR_LOG_WARNING("actor tried to use a blocking function");
        // when using receive(), the non-blocking nature of event-based
        // actors breaks any assumption the user has about his code,
        // in particular, receive_loop() is a deadlock when not throwing
        // an exception here
        aout(this) << "*** warning: event-based actor killed because it tried "
                      "to use receive()\n";
        throw actor_exited(reason);
    }
    planned_exit_reason(reason);
}

message_id local_actor::timed_sync_send_tuple_impl(message_priority mp,
                                                   const actor& dest,
                                                   const util::duration& rtime,
                                                   message&& what) {
    auto nri = new_request_id();
    if (mp == message_priority::high) nri = nri.with_high_priority();
    dest->enqueue({address(), dest, nri}, std::move(what), m_host);
    auto rri = nri.response_id();
    get_scheduling_coordinator()->delayed_send({address(), this, rri}, rtime,
                                  make_message(sync_timeout_msg{}));
    return rri;
}

message_id local_actor::sync_send_tuple_impl(message_priority mp,
                                             const actor& dest,
                                             message&& what) {
    auto nri = new_request_id();
    if (mp == message_priority::high) nri = nri.with_high_priority();
    dest->enqueue({address(), dest, nri}, std::move(what), m_host);
    return nri.response_id();
}

void anon_send_exit(const actor_addr& whom, std::uint32_t reason) {
    auto ptr = detail::raw_access::get(whom);
    ptr->enqueue({invalid_actor_addr, ptr, message_id{}.with_high_priority()},
                 make_message(exit_msg{invalid_actor_addr, reason}), nullptr);
}

} // namespace actor
} // namespace boost
