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

#ifndef BOOST_ACTOR_DETAIL_IMPLICIT_CONVERSIONS_HPP
#define BOOST_ACTOR_DETAIL_IMPLICIT_CONVERSIONS_HPP

#include <string>
#include <type_traits>

#include "boost/actor/fwd.hpp"
#include "boost/actor/detail/type_traits.hpp"

namespace boost {
namespace actor {
namespace detail {

template <class T>
struct implicit_conversions {
  // convert C strings to std::string if possible
  using step1 =
    typename replace_type<
      T, std::string,
      std::is_same<T, const char*>, std::is_same<T, char*>,
      std::is_same<T, char[]>, is_array_of<T, char>,
      is_array_of<T, const char>
    >::type;
  // convert C strings to std::u16string if possible
  using step2 =
    typename replace_type<
      step1, std::u16string,
      std::is_same<step1, const char16_t*>, std::is_same<step1, char16_t*>,
      is_array_of<step1, char16_t>
    >::type;
  // convert C strings to std::u32string if possible
  using step3 =
    typename replace_type<
      step2, std::u32string,
      std::is_same<step2, const char32_t*>, std::is_same<step2, char32_t*>,
      is_array_of<step2, char32_t>
    >::type;
  using type =
    typename replace_type<
      step3, actor,
      std::is_convertible<T, abstract_actor*>, std::is_same<scoped_actor, T>
    >::type;
};

template <class T>
struct strip_and_convert {
  using type =
    typename implicit_conversions<
      typename std::decay<T>::type
    >::type;
};

} // namespace detail
} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_DETAIL_IMPLICIT_CONVERSIONS_HPP
