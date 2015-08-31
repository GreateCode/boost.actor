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

#ifndef BOOST_ACTOR_UNIT_HPP
#define BOOST_ACTOR_UNIT_HPP

#include "boost/actor/detail/comparable.hpp"

namespace boost {
namespace actor {

struct unit_t : detail::comparable<unit_t> {
  constexpr unit_t() {
    // nop
  }

  constexpr unit_t(const unit_t&) {
    // nop
  }

  template <class T>
  explicit constexpr unit_t(T&&) {
    // nop
  }

  static constexpr int compare(const unit_t&) {
    return 0;
  }
};

static constexpr unit_t unit = unit_t{};

template <class T>
struct lift_void {
  using type = T;
};

template <>
struct lift_void<void> {
  using type = unit_t;
};

template <class T>
struct unlift_void {
  using type = T;
};

template <>
struct unlift_void<unit_t> {
  using type = void;
};

} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_UNIT_HPP
