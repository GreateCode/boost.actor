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


#ifndef BOOST_ACTOR_PARTIAL_FUNCTION_HPP
#define BOOST_ACTOR_PARTIAL_FUNCTION_HPP

#include <list>
#include <vector>
#include <memory>
#include <utility>
#include <type_traits>

#include "boost/none.hpp"
#include "boost/intrusive_ptr.hpp"

#include "boost/actor/on.hpp"
#include "boost/actor/message.hpp"
#include "boost/actor/duration.hpp"
#include "boost/actor/behavior.hpp"
#include "boost/actor/ref_counted.hpp"
#include "boost/actor/may_have_timeout.hpp"
#include "boost/actor/timeout_definition.hpp"

#include "boost/actor/detail/behavior_impl.hpp"

namespace boost {
namespace actor {

class behavior;

/**
 * @brief A partial function implementation
 *        for {@link cppa::message messages}.
 */
class message_handler {

    friend class behavior;

 public:

    /** @cond PRIVATE */

    typedef intrusive_ptr<detail::behavior_impl> impl_ptr;

    inline auto as_behavior_impl() const -> impl_ptr;

    message_handler(impl_ptr ptr);

    /** @endcond */

    message_handler() = default;
    message_handler(message_handler&&) = default;
    message_handler(const message_handler&) = default;
    message_handler& operator=(message_handler&&) = default;
    message_handler& operator=(const message_handler&) = default;

    template<typename T, typename... Ts>
    message_handler(const T& arg, Ts&&... args);

    /**
     * @brief Returns a value if @p arg was matched by one of the
     *        handler of this behavior, returns @p nothing otherwise.
     */
    template<typename T>
    inline optional<message> operator()(T&& arg);

    /**
     * @brief Adds a fallback which is used where
     *        this partial function is not defined.
     */
    template<typename... Ts>
    typename std::conditional<
        detail::disjunction<
            may_have_timeout<typename detail::rm_const_and_ref<Ts>::type>::value...
        >::value,
        behavior,
        message_handler
    >::type
    or_else(Ts&&... args) const;

 private:

    impl_ptr m_impl;

};

template<typename... Cases>
message_handler operator,(const match_expr<Cases...>& mexpr,
                           const message_handler& pfun) {
    return mexpr.as_behavior_impl()->or_else(pfun.as_behavior_impl());
}

template<typename... Cases>
message_handler operator,(const message_handler& pfun,
                           const match_expr<Cases...>& mexpr) {
    return pfun.as_behavior_impl()->or_else(mexpr.as_behavior_impl());
}

/******************************************************************************
 *             inline and template member function implementations            *
 ******************************************************************************/

template<typename T, typename... Ts>
message_handler::message_handler(const T& arg, Ts&&... args)
: m_impl(detail::match_expr_concat(
             detail::lift_to_match_expr(arg),
             detail::lift_to_match_expr(std::forward<Ts>(args))...)) { }

template<typename T>
inline optional<message> message_handler::operator()(T&& arg) {
    return (m_impl) ? m_impl->invoke(std::forward<T>(arg)) : none;
}

template<typename... Ts>
typename std::conditional<
    detail::disjunction<
        may_have_timeout<typename detail::rm_const_and_ref<Ts>::type>::value...
    >::value,
    behavior,
    message_handler
>::type
message_handler::or_else(Ts&&... args) const {
    // using a behavior is safe here, because we "cast"
    // it back to a message_handler when appropriate
    behavior tmp{std::forward<Ts>(args)...};
    return m_impl->or_else(tmp.as_behavior_impl());
}

inline auto message_handler::as_behavior_impl() const -> impl_ptr {
    return m_impl;
}

} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_PARTIAL_FUNCTION_HPP