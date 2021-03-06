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

#ifndef BOOST_ACTOR_DETAIL_PSEUDO_TUPLE_HPP
#define BOOST_ACTOR_DETAIL_PSEUDO_TUPLE_HPP

#include <cstddef>

#include "boost/actor/detail/type_traits.hpp"

namespace boost {
namespace actor {
namespace detail {

// tuple-like access to an array of void pointers
template <class... T>
struct pseudo_tuple {
  using pointer = void*;
  using const_pointer = const void*;

  pointer data[sizeof...(T) > 0 ? sizeof...(T) : 1];

  inline const_pointer at(size_t p) const { return data[p]; }

  inline pointer mutable_at(size_t p) { return data[p]; }

  inline pointer& operator[](size_t p) { return data[p]; }

};

template <size_t N, class... Ts>
const typename detail::type_at<N, Ts...>::type&
get(const detail::pseudo_tuple<Ts...>& tv) {
  static_assert(N < sizeof...(Ts), "N >= tv.size()");
  auto vp = tv.at(N);
  BOOST_ACTOR_ASSERT(vp != nullptr);
  return *reinterpret_cast<const typename detail::type_at<N, Ts...>::type*>(vp);
}

template <size_t N, class... Ts>
typename detail::type_at<N, Ts...>::type& get(detail::pseudo_tuple<Ts...>& tv) {
  static_assert(N < sizeof...(Ts), "N >= tv.size()");
  auto vp = tv.mutable_at(N);
  BOOST_ACTOR_ASSERT(vp != nullptr);
  return *reinterpret_cast<typename detail::type_at<N, Ts...>::type*>(vp);
}

} // namespace detail
} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_DETAIL_PSEUDO_TUPLE_HPP
