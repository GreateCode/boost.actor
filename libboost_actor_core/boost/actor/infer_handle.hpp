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

#ifndef BOOST_ACTOR_INFER_HANDLE_HPP
#define BOOST_ACTOR_INFER_HANDLE_HPP

#include "boost/actor/actor.hpp"
#include "boost/actor/actor_addr.hpp"
#include "boost/actor/stateful_actor.hpp"
#include "boost/actor/typed_behavior.hpp"


namespace boost {
namespace actor {

template <class... Sigs>
class typed_event_based_actor;

namespace io {
namespace experimental {

template <class... Sigs>
class typed_broker;

} // namespace experimental
} // namespace io

enum class spawn_mode {
  function,
  function_with_selfptr,
  clazz
};

template <spawn_mode X>
using spawn_mode_token = std::integral_constant<spawn_mode, X>;

// default: dynamically typed actor without self pointer
template <class Result,
          class FirstArg,
          bool FirstArgValid =
            std::is_base_of<
              local_actor,
              typename std::remove_pointer<FirstArg>::type
            >::value>
struct infer_handle_from_fun_impl {
  using type = actor;
  using impl = event_based_actor;
  using behavior_type = behavior;
  static constexpr spawn_mode mode = spawn_mode::function;
};

// dynamically typed actor returning a behavior
template <class Impl>
struct infer_handle_from_fun_impl<void, Impl*, true> {
  using type = actor;
  using impl = Impl;
  using behavior_type = behavior;
  static constexpr spawn_mode mode = spawn_mode::function_with_selfptr;
};

// dynamically typed actor with self pointer
template <class Impl>
struct infer_handle_from_fun_impl<behavior, Impl*, true> {
  using type = actor;
  using impl = Impl;
  using behavior_type = behavior;
  static constexpr spawn_mode mode = spawn_mode::function_with_selfptr;
};

// statically typed actor returning a behavior
template <class... Sigs, class FirstArg>
struct infer_handle_from_fun_impl<typed_behavior<Sigs...>, FirstArg, false> {
  using type = typed_actor<Sigs...>;
  using impl = typed_event_based_actor<Sigs...>;
  using behavior_type = typed_behavior<Sigs...>;
  static constexpr spawn_mode mode = spawn_mode::function;
};

// statically typed actor with self pointer
template <class Result, class... Sigs>
struct infer_handle_from_fun_impl<Result, typed_event_based_actor<Sigs...>*,
                                  true> {
  using type = typed_actor<Sigs...>;
  using impl = typed_event_based_actor<Sigs...>;
  using behavior_type = typed_behavior<Sigs...>;
  static constexpr spawn_mode mode = spawn_mode::function_with_selfptr;
};

// statically typed stateful actor with self pointer
template <class Result, class State, class... Sigs>
struct infer_handle_from_fun_impl<Result,
                                  stateful_actor<
                                    State, typed_event_based_actor<Sigs...>
                                  >*,
                                  true> {
  using type = typed_actor<Sigs...>;
  using impl = stateful_actor<State, typed_event_based_actor<Sigs...>>;
  using behavior_type = typed_behavior<Sigs...>;
  static constexpr spawn_mode mode = spawn_mode::function_with_selfptr;
};

// statically typed broker with self pointer
template <class Result, class... Sigs>
struct infer_handle_from_fun_impl<Result,
                                  io::experimental::typed_broker<Sigs...>*,
                                  true> {
  using type = typed_actor<Sigs...>;
  using impl = io::experimental::typed_broker<Sigs...>;
  using behavior_type = typed_behavior<Sigs...>;
  static constexpr spawn_mode mode = spawn_mode::function_with_selfptr;
};

// statically typed stateful broker with self pointer
template <class Result, class State, class... Sigs>
struct infer_handle_from_fun_impl<Result,
                                  stateful_actor<
                                    State,
                                    io::experimental::typed_broker<Sigs...>
                                  >*,
                                  true> {
  using type = typed_actor<Sigs...>;
  using impl =
    stateful_actor<
      State,
      io::experimental::typed_broker<Sigs...>
    >;
  using behavior_type = typed_behavior<Sigs...>;
  static constexpr spawn_mode mode = spawn_mode::function_with_selfptr;
};

template <class F, class Trait = typename detail::get_callable_trait<F>::type>
struct infer_handle_from_fun {
  using result_type = typename Trait::result_type;
  using arg_types = typename Trait::arg_types;
  using first_arg = typename detail::tl_head<arg_types>::type;
  using delegate = infer_handle_from_fun_impl<result_type, first_arg>;
  using type = typename delegate::type;
  using impl = typename delegate::impl;
  using behavior_type = typename delegate::behavior_type;
  using fun_type = typename Trait::fun_type;
  static constexpr spawn_mode mode = delegate::mode;
};

template <class T>
struct infer_handle_from_behavior {
  using type = actor;
};

template <class... Sigs>
struct infer_handle_from_behavior<typed_behavior<Sigs...>> {
  using type = typed_actor<Sigs...>;
};

template <class T>
struct infer_handle_from_class {
  using type =
    typename infer_handle_from_behavior<
      typename T::behavior_type
    >::type;
  static constexpr spawn_mode mode = spawn_mode::clazz;
};

} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_INFER_HANDLE_HPP

