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

#ifndef BOOST_ACTOR_MAKE_COUNTED_HPP
#define BOOST_ACTOR_MAKE_COUNTED_HPP

#include "boost/actor/ref_counted.hpp"
#include "boost/intrusive_ptr.hpp"

#include "boost/actor/detail/memory.hpp"
#include "boost/actor/detail/type_traits.hpp"

namespace boost {
namespace actor {

/// Constructs an object of type `T` in an `intrusive_ptr`.
/// @relates ref_counted
/// @relates intrusive_ptr
template <class T, class... Ts>
typename std::enable_if<
  detail::is_memory_cached<T>::value,
  intrusive_ptr<T>
>::type
make_counted(Ts&&... xs) {
  return intrusive_ptr<T>(detail::memory::create<T>(std::forward<Ts>(xs)...),
                          false);
}

/// Constructs an object of type `T` in an `intrusive_ptr`.
/// @relates ref_counted
/// @relates intrusive_ptr
template <class T, class... Ts>
typename std::enable_if<
  ! detail::is_memory_cached<T>::value,
  intrusive_ptr<T>
>::type
make_counted(Ts&&... xs) {
  return intrusive_ptr<T>(new T(std::forward<Ts>(xs)...), false);
}

} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_MAKE_COUNTED_HPP
