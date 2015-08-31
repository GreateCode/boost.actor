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

#ifndef BOOST_ACTOR_TYPE_NAME_ACCESS_HPP
#define BOOST_ACTOR_TYPE_NAME_ACCESS_HPP

#include <string>

#include "boost/actor/uniform_typeid.hpp"
#include "boost/actor/uniform_type_info.hpp"

#include "boost/actor/detail/type_traits.hpp"

namespace boost {
namespace actor {

template <class T>
std::string type_name_access_impl(std::true_type) {
  return T::static_type_name();
}

template <class T>
std::string type_name_access_impl(std::false_type) {
  auto uti = uniform_typeid<T>(true);
  return uti ? uti->name() : "void";
}

template <class T>
std::string type_name_access() {
  std::integral_constant<bool, detail::has_static_type_name<T>::value> token;
  return type_name_access_impl<T>(token);
}

} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_TYPE_NAME_ACCESS_HPP
