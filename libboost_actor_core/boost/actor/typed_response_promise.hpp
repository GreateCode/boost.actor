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

#ifndef BOOST_ACTOR_TYPED_RESPONSE_PROMISE_HPP
#define BOOST_ACTOR_TYPED_RESPONSE_PROMISE_HPP

#include "boost/actor/either.hpp"
#include "boost/actor/response_promise.hpp"

namespace boost {
namespace actor {

template <class... Ts>
class typed_response_promise {
public:
  typed_response_promise() = default;
  typed_response_promise(const typed_response_promise&) = default;
  typed_response_promise& operator=(const typed_response_promise&) = default;

  typed_response_promise(response_promise promise) : promise_(promise) {
    // nop
  }

  explicit operator bool() const {
    // handle is valid if it has a receiver
    return static_cast<bool>(promise_);
  }

  template <class... Us>
  void deliver(Us&&... xs) const {
    promise_.deliver(make_message(std::forward<Us>(xs)...));
  }

private:
  response_promise promise_;
};

template <class L, class R>
class typed_response_promise<either_or_t<L, R>> {
public:
  typed_response_promise() = default;
  typed_response_promise(const typed_response_promise&) = default;
  typed_response_promise& operator=(const typed_response_promise&) = default;

  typed_response_promise(response_promise promise) : promise_(promise) {
    // nop
  }

  explicit operator bool() const {
    // handle is valid if it has a receiver
    return static_cast<bool>(promise_);
  }

  template <class... Ts>
  void deliver(Ts&&... xs) const {
    either_or_t<L, R> tmp{std::forward<Ts>(xs)...};
    promise_.deliver(std::move(tmp.value));
  }

private:
  response_promise promise_;
};

} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_TYPED_RESPONSE_PROMISE_HPP
