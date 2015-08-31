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

#ifndef BOOST_ACTOR_TIMEOUT_DEFINITION_HPP
#define BOOST_ACTOR_TIMEOUT_DEFINITION_HPP

#include <functional>

#include "boost/actor/duration.hpp"

namespace boost {
namespace actor {

namespace detail {

class behavior_impl;

behavior_impl* new_default_behavior(duration d, std::function<void()> fun);

} // namespace detail

template <class F>
struct timeout_definition {
  static constexpr bool may_have_timeout = true;
  duration timeout;
  F handler;
  detail::behavior_impl* as_behavior_impl() const {
    return detail::new_default_behavior(timeout, handler);
  }
};

using generic_timeout_definition = timeout_definition<std::function<void()>>;

} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_TIMEOUT_DEFINITION_HPP
