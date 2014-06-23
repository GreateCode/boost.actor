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


#ifndef BOOST_ACTOR_ACTOR_PROXY_HPP
#define BOOST_ACTOR_ACTOR_PROXY_HPP

#include <atomic>
#include <cstdint>

#include "boost/actor/abstract_actor.hpp"

#include "boost/actor/detail/shared_spinlock.hpp"

namespace boost {
namespace actor {

class actor_proxy;

/**
 * @brief A smart pointer to an {@link actor_proxy} instance.
 * @relates actor_proxy
 */
using actor_proxy_ptr = intrusive_ptr<actor_proxy>;

/**
 * @brief Represents a remote actor.
 * @extends abstract_actor
 */
class actor_proxy : public abstract_actor {

    typedef abstract_actor super;

 public:

    /**
     * @brief An anchor points to a proxy instance without sharing
     *        ownership to it, i.e., models a weak ptr.
     */
    class anchor : public ref_counted {

        friend class actor_proxy;

     public:

        anchor(actor_proxy* instance = nullptr);

        ~anchor();

        /**
         * @brief Queries whether the proxy was already deleted.
         */
        bool expired() const;

        /**
         * @brief Gets a pointer to the proxy or @p nullptr
         *        if the instance is {@link expired()}.
         */
        actor_proxy_ptr get();

     private:

        /*
         * @brief Tries to expire this anchor. Fails if reference
         *        count of the proxy is nonzero.
         */
        bool try_expire();

        std::atomic<actor_proxy*> m_ptr;

        detail::shared_spinlock m_lock;

    };

    using anchor_ptr = intrusive_ptr<anchor>;

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
     * @brief
     */
    virtual void kill_proxy(uint32_t reason) = 0;

    void request_deletion() override;

    inline anchor_ptr get_anchor() {
        return m_anchor;
    }

 protected:

    actor_proxy(actor_id aid, node_id nid);

    anchor_ptr m_anchor;

};

} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_ACTOR_PROXY_HPP
