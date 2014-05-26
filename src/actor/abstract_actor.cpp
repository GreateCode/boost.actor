/******************************************************************************\
 *                                                                            *
 *           ____                  _        _        _                        *
 *          | __ )  ___   ___  ___| |_     / \   ___| |_ ___  _ __            *
 *          |  _ \ / _ \ / _ \/ __| __|   / _ \ / __| __/ _ \| '__|           *
 *          | |_) | (_) | (_) \__ \ |_ _ / ___ \ (__| || (_) | |              *
 *          |____/ \___/ \___/|___/\__(_)_/   \_\___|\__\___/|_|              *
 *                                                                            *
 *                                                                            *
 *                                                                            *
 * Copyright (C) 2011 - 2014                                                  *
 * Dominik Charousset <dominik.charousset (at) haw-hamburg.de>                *
 *                                                                            *
 * Distributed under the Boost Software License, Version 1.0. See             *
 * accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt  *
\******************************************************************************/


#include "boost/actor/config.hpp"

#include <map>
#include <mutex>
#include <atomic>
#include <stdexcept>

#include "boost/actor/atom.hpp"
#include "boost/actor/config.hpp"
#include "boost/actor/message.hpp"
#include "boost/actor/actor_addr.hpp"
#include "boost/actor/abstract_actor.hpp"
#include "boost/actor/message_header.hpp"
#include "boost/actor/system_messages.hpp"

#include "boost/actor/detail/logging.hpp"
#include "boost/actor/detail/raw_access.hpp"
#include "boost/actor/detail/singletons.hpp"
#include "boost/actor/detail/actor_registry.hpp"
#include "boost/actor/detail/shared_spinlock.hpp"

namespace boost {
namespace actor {

namespace { typedef std::unique_lock<std::mutex> guard_type; }

// m_exit_reason is guaranteed to be set to 0, i.e., exit_reason::not_exited,
// by std::atomic<> constructor

abstract_actor::abstract_actor(actor_id aid)
        : m_id(aid), m_is_proxy(true)
        , m_exit_reason(exit_reason::not_exited), m_host(nullptr) { }

abstract_actor::abstract_actor()
        : m_id(detail::singletons::get_actor_registry()->next_id())
        , m_is_proxy(false)
        , m_exit_reason(exit_reason::not_exited), m_host(nullptr) {
    m_node = detail::singletons::get_node_id();
}

bool abstract_actor::link_to_impl(const actor_addr& other) {
    if (other && other != this) {
        guard_type guard{m_mtx};
        auto ptr = detail::raw_access::get(other);
        // send exit message if already exited
        if (exited()) {
            ptr->enqueue({address(), ptr},
                         make_message(exit_msg{address(), exit_reason()}),
                         m_host);
        }
        // add link if not already linked to other
        // (checked by establish_backlink)
        else if (ptr->establish_backlink(address())) {
            m_links.push_back(ptr);
            return true;
        }
    }
    return false;
}

bool abstract_actor::attach(attachable_ptr ptr) {
    if (ptr == nullptr) {
        guard_type guard{m_mtx};
        return m_exit_reason == exit_reason::not_exited;
    }
    std::uint32_t reason;
    { // lifetime scope of guard
        guard_type guard{m_mtx};
        reason = m_exit_reason;
        if (reason == exit_reason::not_exited) {
            m_attachables.push_back(std::move(ptr));
            return true;
        }
    }
    ptr->actor_exited(reason);
    return false;
}

void abstract_actor::detach(const attachable::token& what) {
    attachable_ptr ptr;
    { // lifetime scope of guard
        guard_type guard{m_mtx};
        auto end = m_attachables.end();
        auto i = std::find_if(
                    m_attachables.begin(), end,
                    [&](attachable_ptr& p) { return p->matches(what); });
        if (i != end) {
            ptr = std::move(*i);
            m_attachables.erase(i);
        }
    }
    // ptr will be destroyed here, without locked mutex
}

void abstract_actor::link_to(const actor_addr& other) {
    static_cast<void>(link_to_impl(other));
}

void abstract_actor::unlink_from(const actor_addr& other) {
    static_cast<void>(unlink_from_impl(other));
}

bool abstract_actor::remove_backlink(const actor_addr& other) {
    if (other && other != this) {
        guard_type guard{m_mtx};
        auto i = std::find(m_links.begin(), m_links.end(), other);
        if (i != m_links.end()) {
            m_links.erase(i);
            return true;
        }
    }
    return false;
}

bool abstract_actor::establish_backlink(const actor_addr& other) {
    std::uint32_t reason = exit_reason::not_exited;
    if (other && other != this) {
        guard_type guard{m_mtx};
        reason = m_exit_reason;
        if (reason == exit_reason::not_exited) {
            auto i = std::find(m_links.begin(), m_links.end(), other);
            if (i == m_links.end()) {
                m_links.push_back(detail::raw_access::get(other));
                return true;
            }
        }
    }
    // send exit message without lock
    if (reason != exit_reason::not_exited) {
        auto ptr = detail::raw_access::unsafe_cast(other);
        ptr->enqueue({address(), ptr},
                     make_message(exit_msg{address(), exit_reason()}),
                     m_host);
    }
    return false;
}

bool abstract_actor::unlink_from_impl(const actor_addr& other) {
    if (!other) return false;
    guard_type guard{m_mtx};
    // remove_backlink returns true if this actor is linked to other
    auto ptr = detail::raw_access::get(other);
    if (!exited() && ptr->remove_backlink(address())) {
        auto i = std::find(m_links.begin(), m_links.end(), ptr);
        BOOST_ACTOR_REQUIRE(i != m_links.end());
        m_links.erase(i);
        return true;
    }
    return false;
}

actor_addr abstract_actor::address() const {
    return actor_addr{const_cast<abstract_actor*>(this)};
}

void abstract_actor::cleanup(std::uint32_t reason) {
    // log as 'actor'
    BOOST_ACTOR_LOGM_TRACE("cppa::actor", BOOST_ACTOR_ARG(m_id) << ", " << BOOST_ACTOR_ARG(reason)
                    << ", " << BOOST_ACTOR_ARG(m_is_proxy));
    BOOST_ACTOR_REQUIRE(reason != exit_reason::not_exited);
    // move everyhting out of the critical section before processing it
    decltype(m_links) mlinks;
    decltype(m_attachables) mattachables;
    { // lifetime scope of guard
        guard_type guard{m_mtx};
        if (m_exit_reason != exit_reason::not_exited) {
            // already exited
            return;
        }
        m_exit_reason = reason;
        mlinks = std::move(m_links);
        mattachables = std::move(m_attachables);
        // make sure lists are empty
        m_links.clear();
        m_attachables.clear();
    }
    BOOST_ACTOR_LOGC_INFO_IF(not is_proxy(), "cppa::actor", __func__,
                      "actor with ID " << m_id << " had " << mlinks.size()
                      << " links and " << mattachables.size()
                      << " attached functors; exit reason = " << reason
                      << ", class = " << detail::demangle(typeid(*this)));
    // send exit messages
    auto msg = make_message(exit_msg{address(), reason});
    BOOST_ACTOR_LOGM_DEBUG("cppa::actor", "send EXIT to " << mlinks.size() << " links");
    for (auto& aptr : mlinks) {
        aptr->enqueue({address(), aptr, message_id{}.with_high_priority()},
                      msg, m_host);
    }
    BOOST_ACTOR_LOGM_DEBUG("cppa::actor", "run " << mattachables.size()
                                   << " attachables");
    for (attachable_ptr& ptr : mattachables) {
        ptr->actor_exited(reason);
    }
}

std::set<std::string> abstract_actor::interface() const {
    // defaults to untyped
    return std::set<std::string>{};
}

} // namespace actor
} // namespace boost
