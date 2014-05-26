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


#ifndef BOOST_ACTOR_MIXIN_SYNC_SENDER_HPP
#define BOOST_ACTOR_MIXIN_SYNC_SENDER_HPP

#include <tuple>

#include "boost/actor/actor.hpp"
#include "boost/actor/message.hpp"
#include "boost/actor/duration.hpp"
#include "boost/actor/response_handle.hpp"
#include "boost/actor/message_priority.hpp"

namespace boost {
namespace actor {
namespace mixin {

template<class Base, class Subtype, class ResponseHandleTag>
class sync_sender_impl : public Base {

 public:

    typedef response_handle<Subtype,
                            message,
                            ResponseHandleTag>
            response_handle_type;

    /**************************************************************************
     *                     sync_send[_tuple](actor, ...)                      *
     **************************************************************************/

    /**
     * @brief Sends @p what as a synchronous message to @p whom.
     * @param dest Receiver of the message.
     * @param what Message content as tuple.
     * @returns A handle identifying a future to the response of @p whom.
     * @warning The returned handle is actor specific and the response to the
     *          sent message cannot be received by another actor.
     * @throws std::invalid_argument if <tt>whom == nullptr</tt>
     */
    response_handle_type sync_send_tuple(message_priority prio,
                                         const actor& dest,
                                         message what) {
        return {dptr()->sync_send_tuple_impl(prio, dest, std::move(what)),
                dptr()};
    }

    response_handle_type sync_send_tuple(const actor& dest, message what) {
        return sync_send_tuple(message_priority::normal, dest, std::move(what));
    }

    /**
     * @brief Sends <tt>{what...}</tt> as a synchronous message to @p whom.
     * @param dest Receiver of the message.
     * @param what Message elements.
     * @returns A handle identifying a future to the response of @p whom.
     * @warning The returned handle is actor specific and the response to the
     *          sent message cannot be received by another actor.
     * @pre <tt>sizeof...(Ts) > 0</tt>
     * @throws std::invalid_argument if <tt>whom == nullptr</tt>
     */
    template<typename... Ts>
    response_handle_type sync_send(message_priority prio,
                                   const actor& dest,
                                   Ts&&... what) {
        static_assert(sizeof...(Ts) > 0, "no message to send");
        return sync_send_tuple(prio, dest,
                               make_message(std::forward<Ts>(what)...));
    }

    template<typename... Ts>
    response_handle_type sync_send(const actor& dest, Ts&&... what) {
        static_assert(sizeof...(Ts) > 0, "no message to send");
        return sync_send_tuple(message_priority::normal,
                               dest, make_message(std::forward<Ts>(what)...));
    }

    /**************************************************************************
     *                  timed_sync_send[_tuple](actor, ...)                   *
     **************************************************************************/

    response_handle_type timed_sync_send_tuple(message_priority prio,
                                               const actor& dest,
                                               const duration& rtime,
                                               message what) {
        return {dptr()->timed_sync_send_tuple_impl(prio, dest, rtime,
                                                   std::move(what)),
                dptr()};
    }

    response_handle_type timed_sync_send_tuple(const actor& dest,
                                               const duration& rtime,
                                               message what) {
        return {dptr()->timed_sync_send_tuple_impl(message_priority::normal,
                                                   dest, rtime,
                                                   std::move(what)),
                dptr()};
    }

    template<typename... Ts>
    response_handle_type timed_sync_send(message_priority prio,
                                         const actor& dest,
                                         const duration& rtime,
                                         Ts&&... what) {
        static_assert(sizeof...(Ts) > 0, "no message to send");
        return timed_sync_send_tuple(prio, dest, rtime,
                                     make_message(std::forward<Ts>(what)...));
    }

    template<typename... Ts>
    response_handle_type timed_sync_send(const actor& dest,
                                         const duration& rtime,
                                         Ts&&... what) {
        static_assert(sizeof...(Ts) > 0, "no message to send");
        return timed_sync_send_tuple(message_priority::normal, dest, rtime,
                                     make_message(std::forward<Ts>(what)...));
    }

    /**************************************************************************
     *                sync_send[_tuple](typed_actor<...>, ...)                *
     **************************************************************************/

    template<typename... Rs, typename... Ts>
    response_handle<
        Subtype,
        typename detail::deduce_output_type<
            detail::type_list<Rs...>,
            detail::type_list<Ts...>
        >::type,
        ResponseHandleTag
    >
    sync_send_tuple(message_priority prio,
                    const typed_actor<Rs...>& dest,
                    std::tuple<Ts...> what) {
        return sync_send_impl(prio, dest,
                              detail::type_list<Ts...>{},
                              message::move_from_tuple(std::move(what)));
    }

    template<typename... Rs, typename... Ts>
    response_handle<
        Subtype,
        typename detail::deduce_output_type<
            detail::type_list<Rs...>,
            detail::type_list<Ts...>
        >::type,
        ResponseHandleTag
    >
    sync_send_tuple(const typed_actor<Rs...>& dest, std::tuple<Ts...> what) {
        return sync_send_impl(message_priority::normal, dest,
                              detail::type_list<Ts...>{},
                              message::move_from_tuple(std::move(what)));
    }

    template<typename... Rs, typename... Ts>
    response_handle<
        Subtype,
        typename detail::deduce_output_type<
            detail::type_list<Rs...>,
            typename detail::implicit_conversions<
                typename detail::rm_const_and_ref<
                    Ts
                >::type
            >::type...
        >::type,
        ResponseHandleTag
    >
    sync_send(message_priority prio, const typed_actor<Rs...>& dest, Ts&&... what) {
        return sync_send_impl(prio, dest,
                              detail::type_list<
                                  typename detail::implicit_conversions<
                                      typename detail::rm_const_and_ref<
                                          Ts
                                      >::type
                                  >::type...
                              >{},
                              make_message(std::forward<Ts>(what)...));
    }

    template<typename... Rs, typename... Ts>
    response_handle<
        Subtype,
        typename detail::deduce_output_type<
            detail::type_list<Rs...>,
            detail::type_list<
                typename detail::implicit_conversions<
                    typename detail::rm_const_and_ref<
                        Ts
                    >::type
                >::type...
            >
        >::type,
        ResponseHandleTag
    >
    sync_send(const typed_actor<Rs...>& dest, Ts&&... what) {
        return sync_send_impl(message_priority::normal, dest,
                              detail::type_list<
                                  typename detail::implicit_conversions<
                                      typename detail::rm_const_and_ref<
                                          Ts
                                      >::type
                                  >::type...
                              >{},
                              make_message(std::forward<Ts>(what)...));
    }

    /**************************************************************************
     *             timed_sync_send[_tuple](typed_actor<...>, ...)             *
     **************************************************************************/

    /*
    template<typename... Rs, typename... Ts>
    response_handle<
        Subtype,
        typename detail::deduce_output_type<
            detail::type_list<Rs...>,
            detail::type_list<Ts...>
        >::type,
        ResponseHandleTag
    >
    timed_sync_send_tuple(message_priority prio,
                          const typed_actor<Rs...>& dest,
                          const duration& rtime,
                          cow_tuple<Ts...> what) {
        return {dptr()->timed_sync_send_tuple_impl(prio, dest, rtime,
                                                   std::move(what)),
                dptr()};
    }

    template<typename... Rs, typename... Ts>
    response_handle<
        Subtype,
        typename detail::deduce_output_type<
            detail::type_list<Rs...>,
            detail::type_list<Ts...>
        >::type,
        ResponseHandleTag
    >
    timed_sync_send_tuple(const typed_actor<Rs...>& dest,
                          const duration& rtime,
                          cow_tuple<Ts...> what) {
        return {dptr()->timed_sync_send_tuple_impl(message_priority::normal,
                                                   dest, rtime,
                                                   std::move(what)),
                dptr()};
    }
    */

    template<typename... Rs, typename... Ts>
    response_handle<
        Subtype,
        typename detail::deduce_output_type<
            detail::type_list<Rs...>,
            typename detail::implicit_conversions<
                typename detail::rm_const_and_ref<
                    Ts
                >::type
            >::type...
        >::type,
        ResponseHandleTag
    >
    timed_sync_send(message_priority prio,
                    const typed_actor<Rs...>& dest,
                    const duration& rtime,
                    Ts&&... what) {
        static_assert(sizeof...(Ts) > 0, "no message to send");
        return timed_sync_send_tuple(prio, dest, rtime,
                                     make_message(std::forward<Ts>(what)...));
    }

    template<typename... Rs, typename... Ts>
    response_handle<
        Subtype,
        typename detail::deduce_output_type<
            detail::type_list<Rs...>,
            typename detail::implicit_conversions<
                typename detail::rm_const_and_ref<
                    Ts
                >::type
            >::type...
        >::type,
        ResponseHandleTag
    >
    timed_sync_send(const typed_actor<Rs...>& dest,
                    const duration& rtime,
                    Ts&&... what) {
        static_assert(sizeof...(Ts) > 0, "no message to send");
        return timed_sync_send_tuple(message_priority::normal, dest, rtime,
                                     make_message(std::forward<Ts>(what)...));
    }

 private:

    template<typename... Rs, typename... Ts>
    response_handle<
        Subtype,
        typename detail::deduce_output_type<
            detail::type_list<Rs...>,
            detail::type_list<Ts...>
        >::type,
        ResponseHandleTag
    >
    sync_send_impl(message_priority prio,
                   const typed_actor<Rs...>& dest,
                   detail::type_list<Ts...> token,
                   message&& what) {
        dptr()->check_typed_input(dest, token);
        return {dptr()->sync_send_tuple_impl(prio, dest, std::move(what)),
                dptr()};
    }

    inline Subtype* dptr() {
        return static_cast<Subtype*>(this);
    }

};

template<class ResponseHandleTag>
class sync_sender {

 public:

    template<class Base, class Subtype>
    using impl = sync_sender_impl<Base, Subtype, ResponseHandleTag>;

    /*
    template<class Base, class Subtype>
    class impl : public sync_sender_impl<Base, Subtype, ResponseHandleTag> {

        typedef sync_sender_impl<Base, Subtype, ResponseHandleTag> super;

     public:

        typedef impl combined_type;

        template<typename... Ts>
        impl(Ts&&... args) : super(std::forward<Ts>(args)...) { }

    };
    */

};

} // namespace mixin
} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_MIXIN_SYNC_SENDER_HPP
