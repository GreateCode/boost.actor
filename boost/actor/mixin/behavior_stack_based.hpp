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


#ifndef BOOST_ACTOR_MIXIN_BEHAVIOR_STACK_BASED_HPP
#define BOOST_ACTOR_MIXIN_BEHAVIOR_STACK_BASED_HPP

#include "boost/actor/message_id.hpp"
#include "boost/actor/typed_behavior.hpp"
#include "boost/actor/behavior_policy.hpp"
#include "boost/actor/response_handle.hpp"

#include "boost/actor/mixin/single_timeout.hpp"

#include "boost/actor/detail/behavior_stack.hpp"

namespace boost {
namespace actor {
namespace mixin {

template<class Base, class Subtype, class BehaviorType>
class behavior_stack_based_impl : public single_timeout<Base, Subtype> {

    typedef single_timeout<Base, Subtype> super;

 public:

    /**************************************************************************
     *                        typedefs and constructor                        *
     **************************************************************************/

    typedef BehaviorType behavior_type;

    typedef behavior_stack_based_impl combined_type;

    typedef response_handle<behavior_stack_based_impl,
                            message,
                            nonblocking_response_handle_tag>
            response_handle_type;

    template <typename... Ts>
    behavior_stack_based_impl(Ts&&... vs) : super(std::forward<Ts>(vs)...) { }

    /**************************************************************************
     *                    become() member function family                     *
     **************************************************************************/

    void become(behavior_type bhvr) {
        do_become(std::move(bhvr), true);
    }

    template<bool Discard>
    void become(behavior_policy<Discard>, behavior_type bhvr) {
        do_become(std::move(bhvr), Discard);
    }

    template<typename T, typename... Ts>
    inline typename std::enable_if<
        !is_behavior_policy<typename detail::rm_const_and_ref<T>::type>::value,
        void
    >::type
    become(T&& arg, Ts&&... args) {
        do_become(behavior_type{std::forward<T>(arg),
                                std::forward<Ts>(args)...},
                  true);
    }

    template<bool Discard, typename... Ts>
    void become(behavior_policy<Discard>, Ts&&... args) {
        do_become(behavior_type{std::forward<Ts>(args)...}, Discard);
    }

    inline void unbecome() {
        m_bhvr_stack.pop_async_back();
    }

    /**************************************************************************
     *           convenience member function for stack manipulation           *
     **************************************************************************/

    inline bool has_behavior() const {
        return m_bhvr_stack.empty() == false;
    }

    inline behavior& get_behavior() {
        BOOST_ACTOR_REQUIRE(m_bhvr_stack.empty() == false);
        return m_bhvr_stack.back();
    }

    optional<behavior&> sync_handler(message_id msg_id) override {
        return m_bhvr_stack.sync_handler(msg_id);
    }

    inline void remove_handler(message_id mid) {
        m_bhvr_stack.erase(mid);
    }

    inline detail::behavior_stack& bhvr_stack() {
        return m_bhvr_stack;
    }

    /**************************************************************************
     *           extended timeout handling (handle_timeout mem fun)           *
     **************************************************************************/

    void handle_timeout(behavior& bhvr, std::uint32_t timeout_id) {
        if (this->is_active_timeout(timeout_id)) {
            this->reset_timeout();
            bhvr.handle_timeout();
            // request next timeout if behavior stack is not empty
            // and timeout handler did not set a new timeout, e.g.,
            // by calling become()
            if (!this->has_active_timeout() &&  has_behavior()) {
                this->request_timeout(get_behavior().timeout());
            }
        }
    }

 private:

    void do_become(behavior_type bhvr, bool discard_old) {
        if (discard_old) this->m_bhvr_stack.pop_async_back();
        // since we know we extend single_timeout, we can be sure
        // request_timeout simply resets the timeout when it's invalid
        this->request_timeout(bhvr.timeout());
        this->m_bhvr_stack.push_back(std::move(unbox(bhvr)));
    }

    static inline behavior& unbox(behavior& arg) {
        return arg;
    }

    template<typename... Ts>
    static inline behavior& unbox(typed_behavior<Ts...>& arg) {
        return arg.unbox();
    }

    // utility for getting a pointer-to-derived-type
    Subtype* dptr() {
        return static_cast<Subtype*>(this);
    }

    // utility for getting a const pointer-to-derived-type
    const Subtype* dptr() const {
        return static_cast<const Subtype*>(this);
    }

    // allows actors to keep previous behaviors and enables unbecome()
    detail::behavior_stack m_bhvr_stack;

};


/**
 * @brief Mixin for actors using a stack-based message processing.
 * @note This mixin implicitly includes {@link single_timeout}.
 */
template<class BehaviorType>
class behavior_stack_based {

 public:

    template<class Base, class Subtype>
    class impl : public behavior_stack_based_impl<Base, Subtype, BehaviorType> {

        typedef behavior_stack_based_impl<Base, Subtype, BehaviorType> super;

     public:

        typedef impl combined_type;

        template<typename... Ts>
        impl(Ts&&... args) : super(std::forward<Ts>(args)...) { }

    };

};

} // namespace mixin
} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_MIXIN_BEHAVIOR_STACK_BASED_HPP
