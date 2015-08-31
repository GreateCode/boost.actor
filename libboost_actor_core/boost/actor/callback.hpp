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

#ifndef BOOST_ACTOR_NAMED_CALLBACK_HPP
#define BOOST_ACTOR_NAMED_CALLBACK_HPP

#include "boost/actor/config.hpp"

#include "boost/actor/detail/type_traits.hpp"

// The class `callback` intentionally has no virtual destructor, because
// the lifetime of callback objects is never managed via base pointers.
BOOST_ACTOR_PUSH_NON_VIRTUAL_DTOR_WARNING

namespace boost {
namespace actor {

/// Describes a simple callback, usually implemented via lambda expression.
/// Callbacks are used as "type-safe function objects" wherever an interface
/// requires dynamic dispatching. The alternative would be to store the lambda
/// in a `std::function`, which adds another layer of indirection and
/// requires a heap allocation. With the callback implementation of CAF,
/// the object remains on the stack and does not cause more overhead
/// than necessary.
template <class... Ts>
class callback {
public:
  virtual void operator()(Ts...) = 0;
};

/// Utility class for wrapping a function object of type `Base`.
template <class F, class... Ts>
class callback_impl : public callback<Ts...> {
public:
  callback_impl(F&& f) : f_(std::move(f)) {
    // nop
  }

  callback_impl(callback_impl&&) = default;
  callback_impl& operator=(callback_impl&&) = default;

  void operator()(Ts... xs) override {
    f_(xs...);
  }

private:
  F f_;
};

/// Utility class for selecting a `callback_impl`.
template <class F,
          class Args = typename detail::get_callable_trait<F>::arg_types>
struct select_callback;

template <class F, class... Ts>
struct select_callback<F, detail::type_list<Ts...>> {
  using type = callback_impl<F, Ts...>;
};

/// Creates a callback from a lambda expression.
/// @relates callback
template <class F>
typename select_callback<F>::type make_callback(F fun) {
  return {std::move(fun)};
}

} // namespace actor
} // namespace boost

BOOST_ACTOR_POP_WARNINGS

#endif // BOOST_ACTOR_NAMED_CALLBACK_HPP
