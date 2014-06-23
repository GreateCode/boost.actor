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


#ifndef BOOST_ACTOR_ACTOR_NAMESPACE_HPP
#define BOOST_ACTOR_ACTOR_NAMESPACE_HPP

#include <map>
#include <utility>
#include <functional>

#include "boost/actor/node_id.hpp"
#include "boost/actor/actor_proxy.hpp"

namespace boost {
namespace actor {

class serializer;
class deserializer;

/**
 * @brief Groups a (distributed) set of actors and allows actors
 *        in the same namespace to exchange messages.
 */
class actor_namespace {

 public:

    using key_type = node_id_ptr;

    using key_compare = node_id_ptr_less;

    /**
     * @brief The backend of an actor namespace is responsible for
     *        creating proxy actors.
     */
    class backend {

     public:

        virtual ~backend();

        /**
         * @brief Creates a new proxy instance.
         */
        virtual actor_proxy_ptr make_proxy(const key_type&, actor_id) = 0;

        /**
         * @brief Returns the ID of the local node.
         */
        virtual const key_type& node_ptr() const = 0;

    };

    actor_namespace(backend& mgm);

    /**
     * @brief Writes an actor address to @p sink and adds the actor
     *        to the list of known actors for a later deserialization.
     */
    void write(serializer* sink, const actor_addr& ptr);

    /**
     * @brief Reads an actor address from @p source, creating
     *        addresses for remote actors on the fly if needed.
     */
    actor_addr read(deserializer* source);

    /**
     * @brief A map that stores all proxies for known remote actors.
     */
    typedef std::map<actor_id, actor_proxy::anchor_ptr> proxy_map;

    /**
     * @brief Returns the number of proxies for @p node.
     */
    size_t count_proxies(const key_type& node);

    /**
     * @brief Returns the proxy instance identified by @p node and @p aid
     *        or @p nullptr if the actor either unknown or expired.
     */
    actor_proxy_ptr get(const key_type& node, actor_id aid);

    /**
     * @brief Returns the proxy instance identified by @p node and @p aid
     *        or creates a new (default) proxy instance.
     */
    actor_proxy_ptr get_or_put(const key_type& node, actor_id aid);

    /**
     * @brief Deletes all proxies for @p node.
     */
    void erase(const key_type& node);

    /**
     * @brief Deletes the proxy with id @p aid for @p node.
     */
    void erase(const key_type& node, actor_id aid);

    /**
     * @brief Queries whether there are any proxies left.
     */
    bool empty() const;

 private:

    backend& m_backend;

    std::map<key_type, proxy_map, key_compare> m_proxies;

};

} // namespace actor
} // namespace boost

#endif
