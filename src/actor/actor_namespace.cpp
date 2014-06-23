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


#include <utility>
#include <algorithm>

#include "boost/actor/node_id.hpp"
#include "boost/actor/serializer.hpp"
#include "boost/actor/deserializer.hpp"
#include "boost/actor/actor_namespace.hpp"

#include "boost/actor_io/middleman.hpp"
#include "boost/actor_io/remote_actor_proxy.hpp"

#include "boost/actor/detail/logging.hpp"
#include "boost/actor/detail/singletons.hpp"
#include "boost/actor/detail/actor_registry.hpp"

namespace boost {
namespace actor {

actor_namespace::backend::~backend() {
    // nop
}

actor_namespace::actor_namespace(backend& be) : m_backend(be) {
    // nop
}

void actor_namespace::write(serializer* sink, const actor_addr& addr) {
    BOOST_ACTOR_REQUIRE(sink != nullptr);
    if (!addr) {
        node_id::host_id_type zero;
        std::fill(zero.begin(), zero.end(), 0);
        sink->write_value(static_cast<actor_id>(0));         // actor id
        sink->write_value(static_cast<uint32_t>(0));         // process id
        sink->write_raw(node_id::host_id_size, zero.data()); // host id
    }
    else {
        // register locally running actors to be able to deserialize them later
        if (!addr.is_remote()) {
            detail::singletons::get_actor_registry()
            ->put(addr.id(), actor_cast<abstract_actor_ptr>(addr));
        }
        auto& pinf = addr.node();
        sink->write_value(addr.id());                                  // actor id
        sink->write_value(pinf.process_id());                          // process id
        sink->write_raw(node_id::host_id_size, pinf.host_id().data()); // host id
    }
}

actor_addr actor_namespace::read(deserializer* source) {
    BOOST_ACTOR_REQUIRE(source != nullptr);
    node_id::host_id_type hid;
    auto aid = source->read<uint32_t>();                 // actor id
    auto pid = source->read<uint32_t>();                 // process id
    source->read_raw(node_id::host_id_size, hid.data()); // host id
    node_id_ptr this_node = detail::singletons::get_node_id();
    if (aid == 0 && pid == 0) {
        // 0:0 identifies an invalid actor
        return invalid_actor_addr;
    }
    if (pid == this_node->process_id() && hid == this_node->host_id()) {
        // identifies this exact process on this host, ergo: local actor
        auto a = detail::singletons::get_actor_registry()->get(aid);
        // might be invalid
        return a ? a->address() : invalid_actor_addr;

    }
    // identifies a remote actor; create proxy if needed
    node_id_ptr tmp = new node_id{pid, hid};
    return get_or_put(tmp, aid)->address();
}

size_t actor_namespace::count_proxies(const key_type& node) {
    auto i = m_proxies.find(node);
    return (i != m_proxies.end()) ? i->second.size() : 0;
}

actor_proxy_ptr actor_namespace::get(const key_type& node, actor_id aid) {
    auto& submap = m_proxies[node];
    auto i = submap.find(aid);
    if (i != submap.end()) {
        auto res = i->second->get();
        if (!res) submap.erase(i); // instance is expired
        return res;
    }
    return nullptr;
}

actor_proxy_ptr actor_namespace::get_or_put(const key_type& node, actor_id aid) {
    auto& submap = m_proxies[node];
    auto& anchor = submap[aid];
    actor_proxy_ptr result;
    if (anchor) result = anchor->get();
    // replace anchor if we've created one using the default ctor
    // or if we've found an expired one in the map
    if (!anchor || !result) {
        result = m_backend.make_proxy(node, aid);
        anchor = result->get_anchor();
    }
    return result;
}

bool actor_namespace::empty() const {
    return m_proxies.empty();
}

void actor_namespace::erase(const key_type& inf) {
    BOOST_ACTOR_LOG_TRACE(BOOST_ACTOR_TARG(inf, to_string));
    m_proxies.erase(inf);
}

void actor_namespace::erase(const key_type& inf, actor_id aid) {
    BOOST_ACTOR_LOG_TRACE(BOOST_ACTOR_TARG(inf, to_string) << ", " << BOOST_ACTOR_ARG(aid));
    auto i = m_proxies.find(inf);
    if (i != m_proxies.end()) {
        i->second.erase(aid);
        if (i->second.empty()) m_proxies.erase(i);
    }
}

} // namespace actor
} // namespace boost
