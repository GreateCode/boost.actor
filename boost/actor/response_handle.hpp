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


#ifndef BOOST_ACTOR_RESPONSE_HANDLE_HPP
#define BOOST_ACTOR_RESPONSE_HANDLE_HPP

#include <type_traits>

#include "boost/actor/message_id.hpp"
#include "boost/actor/typed_behavior.hpp"
#include "boost/actor/continue_helper.hpp"
#include "boost/actor/typed_continue_helper.hpp"

#include "boost/actor/detail/type_list.hpp"

#include "boost/actor/detail/typed_actor_util.hpp"
#include "boost/actor/detail/response_handle_util.hpp"

namespace boost {
namespace actor {

/**
 * @brief This tag identifies response handles featuring a
 *        nonblocking API by providing a @p then member function.
 * @relates response_handle
 */
struct nonblocking_response_handle_tag { };

/**
 * @brief This tag identifies response handles featuring a
 *        blocking API by providing an @p await member function.
 * @relates response_handle
 */
struct blocking_response_handle_tag { };

/**
 * @brief This helper class identifies an expected response message
 *        and enables <tt>sync_send(...).then(...)</tt>.
 *
 * @tparam Self The type of the actor this handle belongs to.
 * @tparam Result Either message or type_list<R1, R2, ... Rn>.
 * @tparam Tag Either {@link nonblocking_response_handle_tag} or
 *             {@link blocking_response_handle_tag}.
 */
template<class Self, class Result, class Tag>
class response_handle;

/******************************************************************************
 *                           nonblocking + untyped                            *
 ******************************************************************************/
template<class Self>
class response_handle<Self, message, nonblocking_response_handle_tag> {

 public:

    response_handle() = delete;

    response_handle(const response_handle&) = default;

    response_handle& operator=(const response_handle&) = default;

    inline continue_helper then(behavior bhvr) const {
        m_self->bhvr_stack().push_back(std::move(bhvr), m_mid);
        return {m_mid, m_self};
    }

    template<typename... Cs, typename... Ts>
    continue_helper then(const match_expr<Cs...>& arg, const Ts&... args) const {
        return then(behavior{arg, args...});
    }

    template<typename... Fs>
    typename std::enable_if<
        detail::all_callable<Fs...>::value,
        continue_helper
    >::type
    then(Fs... fs) const {
        return then(detail::fs2bhvr(m_self, fs...));
    }

    response_handle(const message_id& mid, Self* self)
            : m_mid(mid), m_self(self) { }

 private:

    message_id m_mid;

    Self* m_self;

};

/******************************************************************************
 *                            nonblocking + typed                             *
 ******************************************************************************/
template<class Self, typename... Ts>
class response_handle<Self, detail::type_list<Ts...>, nonblocking_response_handle_tag> {

 public:

    response_handle() = delete;

    response_handle(const response_handle&) = default;

    response_handle& operator=(const response_handle&) = default;

    template<typename F,
             typename Enable = typename std::enable_if<
                                      detail::is_callable<F>::value
                                   && !is_match_expr<F>::value
                               >::type>
    typed_continue_helper<
        typename detail::lifted_result_type<
            typename detail::get_callable_trait<F>::result_type
        >::type
    >
    then(F fun) {
        detail::assert_types<detail::type_list<Ts...>, F>();
        m_self->bhvr_stack().push_back(behavior{on_arg_match >> fun}, m_mid);
        return {m_mid, m_self};
    }

    response_handle(const message_id& mid, Self* self)
            : m_mid(mid), m_self(self) { }

 private:

    message_id m_mid;

    Self* m_self;

};

/******************************************************************************
 *                             blocking + untyped                             *
 ******************************************************************************/
template<class Self>
class response_handle<Self, message, blocking_response_handle_tag> {

 public:

    response_handle() = delete;

    response_handle(const response_handle&) = default;

    response_handle& operator=(const response_handle&) = default;

    void await(behavior& pfun) {
        m_self->dequeue_response(pfun, m_mid);
    }

    inline void await(behavior&& pfun) {
        behavior tmp{std::move(pfun)};
        await(tmp);
    }

    template<typename... Cs, typename... Ts>
    void await(const match_expr<Cs...>& arg, const Ts&... args) {
        behavior bhvr{arg, args...};
        await(bhvr);
    }

    template<typename... Fs>
    typename std::enable_if<detail::all_callable<Fs...>::value>::type
    await(Fs... fs) {
        await(detail::fs2bhvr(m_self, fs...));
    }

    response_handle(const message_id& mid, Self* self)
            : m_mid(mid), m_self(self) { }

 private:

   message_id m_mid;

   Self* m_self;

};

/******************************************************************************
 *                              blocking + typed                              *
 ******************************************************************************/
template<class Self, typename... Ts>
class response_handle<Self, detail::type_list<Ts...>, blocking_response_handle_tag> {

 public:

    typedef detail::type_list<Ts...> result_types;

    response_handle() = delete;

    response_handle(const response_handle&) = default;

    response_handle& operator=(const response_handle&) = default;

    template<typename F>
    void await(F fun) {
        typedef typename detail::tl_map<
                    typename detail::get_callable_trait<F>::arg_types,
                    detail::rm_const_and_ref
                >::type
                arg_types;
        static constexpr size_t fun_args = detail::tl_size<arg_types>::value;
        static_assert(fun_args <= detail::tl_size<result_types>::value,
                      "functor takes too much arguments");
        typedef typename detail::tl_right<result_types, fun_args>::type recv_types;
        static_assert(std::is_same<arg_types, recv_types>::value,
                      "wrong functor signature");
        behavior tmp = detail::fs2bhvr(m_self, fun);
        m_self->dequeue_response(tmp, m_mid);
    }

    response_handle(const message_id& mid, Self* self)
            : m_mid(mid), m_self(self) { }

 private:

    message_id m_mid;

    Self* m_self;

};

} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_RESPONSE_HANDLE_HPP
