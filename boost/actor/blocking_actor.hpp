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


#ifndef BOOST_ACTOR_BLOCKING_UNTYPED_ACTOR_HPP
#define BOOST_ACTOR_BLOCKING_UNTYPED_ACTOR_HPP

#include "boost/none.hpp"

#include "boost/actor/on.hpp"
#include "boost/actor/extend.hpp"
#include "boost/actor/behavior.hpp"
#include "boost/actor/local_actor.hpp"
#include "boost/actor/typed_actor.hpp"
#include "boost/actor/mailbox_element.hpp"
#include "boost/actor/response_handle.hpp"

#include "boost/actor/mixin/sync_sender.hpp"
#include "boost/actor/mixin/mailbox_based.hpp"

namespace boost {
namespace actor {

/**
 * @brief A thread-mapped or context-switching actor using a blocking
 *        receive rather than a behavior-stack based message processing.
 * @extends local_actor
 */
class blocking_actor
        : public extend<local_actor, blocking_actor>::
                        with<mixin::mailbox_based,
                             mixin::sync_sender<blocking_response_handle_tag>::impl> {

 public:

    /**************************************************************************
     *           utility stuff and receive() member function family           *
     **************************************************************************/
 typedef std::chrono::high_resolution_clock::time_point timeout_type;

    struct receive_while_helper {

        std::function<void(behavior&)> m_dq;
        std::function<bool()> m_stmt;

        template<typename... Ts>
        void operator()(Ts&&... args) {
            static_assert(sizeof...(Ts) > 0,
                          "operator() requires at least one argument");
            behavior bhvr{std::forward<Ts>(args)...};
            while (m_stmt()) m_dq(bhvr);
        }

    };

    template<typename T>
    struct receive_for_helper {

        std::function<void(behavior&)> m_dq;
        T& begin;
        T end;

        template<typename... Ts>
        void operator()(Ts&&... args) {
            behavior bhvr{std::forward<Ts>(args)...};
            for ( ; begin != end; ++begin) m_dq(bhvr);
        }

    };

    struct do_receive_helper {

        std::function<void(behavior&)> m_dq;
        behavior m_bhvr;

        template<typename Statement>
        void until(Statement stmt) {
            do { m_dq(m_bhvr); }
            while (stmt() == false);
        }

    };

    /**
     * @brief Dequeues the next message from the mailbox that
     *        is matched by given behavior.
     */
    template<typename... Ts>
    void receive(Ts&&... args) {
        static_assert(sizeof...(Ts), "at least one argument required");
        behavior bhvr{std::forward<Ts>(args)...};
        dequeue(bhvr);
    }

    /**
     * @brief Receives messages in an endless loop.
     *        Semantically equal to: <tt>for (;;) { receive(...); }</tt>
     */
    template<typename... Ts>
    void receive_loop(Ts&&... args) {
        behavior bhvr{std::forward<Ts>(args)...};
        for (;;) dequeue(bhvr);
    }

    /**
     * @brief Receives messages as in a range-based loop.
     *
     * Semantically equal to:
     * <tt>for ( ; begin != end; ++begin) { receive(...); }</tt>.
     *
     * <b>Usage example:</b>
     * @code
     * int i = 0;
     * receive_for(i, 10) (
     *     on(atom("get")) >> [&]() -> message { return {"result", i}; }
     * );
     * @endcode
     * @param begin First value in range.
     * @param end Last value in range (excluded).
     * @returns A functor implementing the loop.
     */
    template<typename T>
    receive_for_helper<T> receive_for(T& begin, const T& end) {
        return {make_dequeue_callback(), begin, end};
    }

    /**
     * @brief Receives messages as long as @p stmt returns true.
     *
     * Semantically equal to: <tt>while (stmt()) { receive(...); }</tt>.
     *
     * <b>Usage example:</b>
     * @code
     * int i = 0;
     * receive_while([&]() { return (++i <= 10); })
     * (
     *     on<int>() >> int_fun,
     *     on<float>() >> float_fun
     * );
     * @endcode
     * @param stmt Lambda expression, functor or function returning a @c bool.
     * @returns A functor implementing the loop.
     */
    template<typename Statement>
    receive_while_helper receive_while(Statement stmt) {
        static_assert(std::is_same<bool, decltype(stmt())>::value,
                      "functor or function does not return a boolean");
        return {make_dequeue_callback(), stmt};
    }

    /**
     * @brief Receives messages until @p stmt returns true.
     *
     * Semantically equal to:
     * <tt>do { receive(...); } while (stmt() == false);</tt>
     *
     * <b>Usage example:</b>
     * @code
     * int i = 0;
     * do_receive
     * (
     *     on<int>() >> int_fun,
     *     on<float>() >> float_fun
     * )
     * .until([&]() { return (++i >= 10); };
     * @endcode
     * @param args Denotes the actor's response the next incoming message.
     * @returns A functor providing the @c until member function.
     */
    template<typename... Ts>
    do_receive_helper do_receive(Ts&&... args) {
        return {make_dequeue_callback(), behavior{std::forward<Ts>(args)...}};
    }

    optional<behavior&> sync_handler(message_id msg_id) override {
        auto i = m_sync_handler.find(msg_id);
        if (i != m_sync_handler.end()) return i->second;
        return none;
    }

    /**
     * @brief Blocks this actor until all other actors are done.
     */
    void await_all_other_actors_done();

    /**
     * @brief Implements the actor's behavior.
     */
    virtual void act() = 0;

    /**
     * @brief Unwinds the stack by throwing an actor_exited exception.
     * @throws actor_exited
     */
    virtual void quit(std::uint32_t reason = exit_reason::normal);

    /** @cond PRIVATE */

    // required from invoke_policy; unused in blocking actors
    inline void remove_handler(message_id) { }

    // required by receive() member function family
    inline void dequeue(behavior&& bhvr) {
        behavior tmp{std::move(bhvr)};
        dequeue(tmp);
    }

    // required by receive() member function family
    inline void dequeue(behavior& bhvr) {
        dequeue_response(bhvr, message_id::invalid);
    }

    // implemented by detail::proper_actor
    virtual void dequeue_response(behavior& bhvr, message_id mid) = 0;

    /** @endcond */

 private:

    // helper function to implement receive_(for|while) and do_receive
    std::function<void(behavior&)> make_dequeue_callback() {
        return [=](behavior& bhvr) { dequeue(bhvr); };
    }

    std::map<message_id, behavior> m_sync_handler;

};

} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_BLOCKING_UNTYPED_ACTOR_HPP
