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
 * Copyright (C) 2011 - 2015                                                  *
 * Dominik Charousset <dominik.charousset (at) haw-hamburg.de>                *
 *                                                                            *
 * Distributed under the terms and conditions of the BSD 3-Clause License or  *
 * (at your option) under the terms and conditions of the Boost Software      *
 * License 1.0. See accompanying files LICENSE and LICENSE_ALTERNATIVE.       *
 *                                                                            *
 * If you did not receive a copy of the license files, see                    *
 * http://opensource.org/licenses/BSD-3-Clause and                            *
 * http://www.boost.org/LICENSE_1_0.txt.                                      *
 ******************************************************************************/

#ifndef BOOST_ACTOR_ON_HPP
#define BOOST_ACTOR_ON_HPP

#include <chrono>
#include <memory>
#include <functional>
#include <type_traits>

#include "boost/actor/unit.hpp"
#include "boost/actor/atom.hpp"
#include "boost/actor/anything.hpp"
#include "boost/actor/message.hpp"
#include "boost/actor/duration.hpp"
#include "boost/actor/skip_message.hpp"
#include "boost/actor/may_have_timeout.hpp"
#include "boost/actor/timeout_definition.hpp"

#include "boost/actor/detail/boxed.hpp"
#include "boost/actor/detail/unboxed.hpp"
#include "boost/actor/detail/type_list.hpp"
#include "boost/actor/detail/arg_match_t.hpp"
#include "boost/actor/detail/type_traits.hpp"
#include "boost/actor/detail/match_case_builder.hpp"
#include "boost/actor/detail/tail_argument_token.hpp"
#include "boost/actor/detail/implicit_conversions.hpp"

namespace boost {
namespace actor {
namespace detail {

template <class T, bool IsFun = detail::is_callable<T>::value
                                && ! detail::is_boxed<T>::value>
struct pattern_type {
  using type =
    typename implicit_conversions<
      typename std::decay<
        typename detail::unboxed<T>::type
      >::type
    >::type;
};

template <class T>
struct pattern_type<T, true> {
  using ctrait = detail::get_callable_trait<T>;
  using args = typename ctrait::arg_types;
  static_assert(detail::tl_size<args>::value == 1,
                "only unary functions are allowed as projections");
  using type =
    typename std::decay<
      typename detail::tl_head<args>::type
    >::type;
};

} // namespace detail
} // namespace actor
} // namespace boost

namespace boost {
namespace actor {

/// A wildcard that matches any number of any values.
constexpr auto any_vals = anything();

/// A wildcard that matches any value of type `T`.
template <class T>
constexpr typename detail::boxed<T>::type val() {
  return typename detail::boxed<T>::type();
}

/// A wildcard that matches the argument types
/// of a given callback, must be the last argument to `on()`.

constexpr auto arg_match = detail::boxed<detail::arg_match_t>::type();

/// Generates function objects from a binary predicate and a value.
template <class T, typename BinaryPredicate>
std::function<optional<T>(const T&)> guarded(BinaryPredicate p, T value) {
  return [=](const T& other) -> optional<T> {
    if (p(other, value)) {
      return value;
    }
    return none;
  };
}

// special case covering arg_match as argument to guarded()
template <class T, typename Predicate>
unit_t guarded(Predicate, const detail::wrapped<T>&) {
  return unit;
}

inline unit_t to_guard(const anything&) {
  return unit;
}

template <class T>
unit_t to_guard(detail::wrapped<T> (*)()) {
  return unit;
}

template <class T>
unit_t to_guard(const detail::wrapped<T>&) {
  return unit;
}

template <class T>
std::function<optional<typename detail::strip_and_convert<T>::type>(
  const typename detail::strip_and_convert<T>::type&)>
to_guard(const T& value,
         typename std::enable_if<! detail::is_callable<T>::value>::type* = 0) {
  using type = typename detail::strip_and_convert<T>::type;
  return guarded<type>(std::equal_to<type>{}, value);
}

template <class F>
F to_guard(F fun,
           typename std::enable_if<detail::is_callable<F>::value>::type* = 0) {
  return fun;
}

template <atom_value V>
auto to_guard(const atom_constant<V>&) -> decltype(to_guard(V)) {
  return to_guard(V);
}

/// Returns a generator for `match_case` objects.
template <class... Ts>
auto on(const Ts&... xs)
-> detail::advanced_match_case_builder<
    detail::type_list<
      decltype(to_guard(xs))...
    >,
    detail::type_list<
      typename detail::pattern_type<typename std::decay<Ts>::type>::type...>
    > {
  return {detail::variadic_ctor{}, to_guard(xs)...};
}

/// Returns a generator for `match_case` objects.
template <class T, class... Ts>
decltype(on(val<T>(), val<Ts>()...)) on() {
  return on(val<T>(), val<Ts>()...);
}

/// Returns a generator for `match_case` objects.
template <atom_value A0, class... Ts>
decltype(on(A0, val<Ts>()...)) on() {
  return on(A0, val<Ts>()...);
}

/// Returns a generator for `match_case` objects.
template <atom_value A0, atom_value A1, class... Ts>
decltype(on(A0, A1, val<Ts>()...)) on() {
  return on(A0, A1, val<Ts>()...);
}

/// Returns a generator for `match_case` objects.
template <atom_value A0, atom_value A1, atom_value A2, class... Ts>
decltype(on(A0, A1, A2, val<Ts>()...)) on() {
  return on(A0, A1, A2, val<Ts>()...);
}

/// Returns a generator for `match_case` objects.
template <atom_value A0, atom_value A1, atom_value A2, atom_value A3,
          class... Ts>
decltype(on(A0, A1, A2, A3, val<Ts>()...)) on() {
  return on(A0, A1, A2, A3, val<Ts>()...);
}

/// Returns a generator for timeouts.
template <class Rep, class Period>
constexpr detail::timeout_definition_builder
after(const std::chrono::duration<Rep, Period>& d) {
  return {duration(d)};
}

/// Generates catch-all `match_case` objects.
constexpr auto others = detail::catch_all_match_case_builder();

/// Semantically equal to `on(arg_match)`, but uses a (faster)
/// special-purpose `match_case` implementation.
constexpr auto on_arg_match = detail::trivial_match_case_builder();

} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_ON_HPP
