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

#ifndef BOOST_ACTOR_DETAIL_SCOPE_GUARD_HPP
#define BOOST_ACTOR_DETAIL_SCOPE_GUARD_HPP

#include <utility>

namespace boost {
namespace actor {
namespace detail {

/// A lightweight scope guard implementation.
template <class Fun>
class scope_guard {

  scope_guard() = delete;
  scope_guard(const scope_guard&) = delete;
  scope_guard& operator=(const scope_guard&) = delete;

public:

  scope_guard(Fun f) : fun_(std::move(f)), enabled_(true) {}

  scope_guard(scope_guard&& other)
      : fun_(std::move(other.fun_)), enabled_(other.enabled_) {
    other.enabled_ = false;
  }

  ~scope_guard() {
    if (enabled_) fun_();
  }

  /// Disables this guard, i.e., the guard does not
  /// run its cleanup code as it goes out of scope.
  inline void disable() { enabled_ = false; }

private:

  Fun fun_;
  bool enabled_;

};

/// Creates a guard that executes `f` as soon as it goes out of scope.
/// @relates scope_guard
template <class Fun>
scope_guard<Fun> make_scope_guard(Fun f) {
  return {std::move(f)};
}

} // namespace detail
} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_DETAIL_SCOPE_GUARD_HPP
