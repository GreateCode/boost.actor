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

#ifndef BOOST_ACTOR_DETAIL_RUN_PROGRAM_HPP
#define BOOST_ACTOR_DETAIL_RUN_PROGRAM_HPP

#include "boost/actor/send.hpp"
#include "boost/actor/actor.hpp"
#include "boost/actor/message.hpp"
#include <boost/algorithm/string.hpp>

#include <thread>
#include <vector>
#include <string>

namespace boost {
namespace actor {
namespace detail {

template <class T>
typename std::enable_if<
  std::is_arithmetic<T>::value,
  std::string
>::type
convert_to_str(T value) {
  return std::to_string(value);
}

inline std::string convert_to_str(std::string value) {
  return std::move(value);
}

std::thread run_program_impl(boost::actor::actor, const char*, std::vector<std::string>);

template <class... Ts>
std::thread run_program(boost::actor::actor listener, const char* path, Ts&&... args) {
  std::vector<std::string> vec{convert_to_str(std::forward<Ts>(args))...};
  return run_program_impl(listener, path, std::move(vec));
}

} // namespace detail
} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_DETAIL_RUN_PROGRAM_HPP
