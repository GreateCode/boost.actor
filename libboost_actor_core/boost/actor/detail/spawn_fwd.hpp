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

#ifndef BOOST_ACTOR_DETAIL_SPAWN_FWD_HPP
#define BOOST_ACTOR_DETAIL_SPAWN_FWD_HPP

#include <functional>

#include "boost/actor/fwd.hpp"

#include "boost/actor/detail/type_traits.hpp"

namespace boost {
namespace actor {
namespace detail {

/// Converts `scoped_actor` and pointers to actors to handles of type `actor`
/// but simply forwards any other argument in the same way `std::forward` does.
template <class T>
typename std::conditional<
  is_convertible_to_actor<typename std::decay<T>::type>::value,
  actor,
  T&&
>::type
spawn_fwd(typename std::remove_reference<T>::type& arg) noexcept {
  return static_cast<T&&>(arg);
}

/// Converts `scoped_actor` and pointers to actors to handles of type `actor`
/// but simply forwards any other argument in the same way `std::forward` does.
template <class T>
typename std::conditional<
  is_convertible_to_actor<typename std::decay<T>::type>::value,
  actor,
  T&&
>::type
spawn_fwd(typename std::remove_reference<T>::type&& arg) noexcept {
  static_assert(! std::is_lvalue_reference<T>::value,
                "silently converting an lvalue to an rvalue");
  return static_cast<T&&>(arg);
}

} // namespace detail
} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_DETAIL_SPAWN_FWD_HPP
