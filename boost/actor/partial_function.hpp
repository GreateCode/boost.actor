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

#include "boost/actor/on.hpp"
#include "boost/actor/behavior.hpp"
#include "boost/actor/any_tuple.hpp"
#include "boost/actor/ref_counted.hpp"
#include "boost/actor/intrusive_ptr.hpp"
#include "boost/actor/may_have_timeout.hpp"
#include "boost/actor/timeout_definition.hpp"

#include "boost/actor/util/duration.hpp"

#include "boost/actor/detail/behavior_impl.hpp"

namespace boost {
namespace actor {

class behavior;

/**
 * @brief A partial function implementation
 *        for {@link cppa::any_tuple any_tuples}.
 */
class partial_function {

    friend class behavior;

 public:

    /** @cond PRIVATE */

    typedef intrusive_ptr<detail::behavior_impl> impl_ptr;

    inline auto as_behavior_impl() const -> impl_ptr;

    partial_function(impl_ptr ptr);

    /** @endcond */

    partial_function() = default;
    partial_function(partial_function&&) = default;
    partial_function(const partial_function&) = default;
    partial_function& operator=(partial_function&&) = default;
    partial_function& operator=(const partial_function&) = default;

    template<typename T, typename... Ts>
    partial_function(const T& arg, Ts&&... args);

    /**
     * @brief Returns @p true if this partial function is defined for the
     *        types of @p value, false otherwise.
     */
    inline bool defined_at(const any_tuple& value);

    /**
     * @brief Returns a value if @p arg was matched by one of the
     *        handler of this behavior, returns @p nothing otherwise.
     * @note This member function can return @p nothing even if
     *       {@link defined_at()} returns @p true, because {@link defined_at()}
     *       does not evaluate guards.
     */
    template<typename T>
    inline optional<any_tuple> operator()(T&& arg);

    /**
     * @brief Adds a fallback which is used where
     *        this partial function is not defined.
     */
    template<typename... Ts>
    typename std::conditional<
        util::disjunction<
            may_have_timeout<typename util::rm_const_and_ref<Ts>::type>::value...
        >::value,
        behavior,
        partial_function
    >::type
    or_else(Ts&&... args) const;

 private:

    impl_ptr m_impl;

};

template<typename... Cases>
partial_function operator,(const match_expr<Cases...>& mexpr,
                           const partial_function& pfun) {
    return mexpr.as_behavior_impl()->or_else(pfun.as_behavior_impl());
}

template<typename... Cases>
partial_function operator,(const partial_function& pfun,
                           const match_expr<Cases...>& mexpr) {
    return pfun.as_behavior_impl()->or_else(mexpr.as_behavior_impl());
}

/******************************************************************************
 *             inline and template member function implementations            *
 ******************************************************************************/

template<typename T, typename... Ts>
partial_function::partial_function(const T& arg, Ts&&... args)
: m_impl(detail::match_expr_concat(
             detail::lift_to_match_expr(arg),
             detail::lift_to_match_expr(std::forward<Ts>(args))...)) { }

inline bool partial_function::defined_at(const any_tuple& value) {
    return (m_impl) && m_impl->defined_at(value);
}

template<typename T>
inline optional<any_tuple> partial_function::operator()(T&& arg) {
    return (m_impl) ? m_impl->invoke(std::forward<T>(arg)) : none;
}

template<typename... Ts>
typename std::conditional<
    util::disjunction<
        may_have_timeout<typename util::rm_const_and_ref<Ts>::type>::value...
    >::value,
    behavior,
    partial_function
>::type
partial_function::or_else(Ts&&... args) const {
    // using a behavior is safe here, because we "cast"
    // it back to a partial_function when appropriate
    behavior tmp{std::forward<Ts>(args)...};
    return m_impl->or_else(tmp.as_behavior_impl());
}

inline auto partial_function::as_behavior_impl() const -> impl_ptr {
    return m_impl;
}

} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_PARTIAL_FUNCTION_HPP
