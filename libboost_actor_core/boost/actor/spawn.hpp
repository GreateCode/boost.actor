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

#ifndef BOOST_ACTOR_SPAWN_HPP
#define BOOST_ACTOR_SPAWN_HPP

#include <type_traits>

#include "boost/actor/spawn_fwd.hpp"
#include "boost/actor/message_id.hpp"
#include "boost/actor/typed_actor.hpp"
#include "boost/actor/make_counted.hpp"
#include "boost/actor/spawn_options.hpp"
#include "boost/actor/typed_event_based_actor.hpp"

#include "boost/actor/detail/logging.hpp"
#include "boost/actor/detail/spawn_fwd.hpp"
#include "boost/actor/detail/type_traits.hpp"
#include "boost/actor/detail/init_fun_factory.hpp"
#include "boost/actor/detail/typed_actor_util.hpp"
#include "boost/actor/detail/implicit_conversions.hpp"

namespace boost {
namespace actor {

class execution_unit;

/// Returns a newly spawned instance of type `C` using `xs...` as constructor
/// arguments. The instance will be added to the job list of `host`. However,
/// before the instance is launched, `before_launch_fun` will be called, e.g.,
/// to join a group before the actor is running.
template <class C, spawn_options Os, class BeforeLaunch, class... Ts>
intrusive_ptr<C> spawn_impl(execution_unit* host,
                            BeforeLaunch before_launch_fun, Ts&&... xs) {
  static_assert(! std::is_base_of<blocking_actor, C>::value
                || has_blocking_api_flag(Os),
                "C is derived from blocking_actor but "
                "spawned without blocking_api_flag");
  static_assert(is_unbound(Os),
                "top-level spawns cannot have monitor or link flag");
  BOOST_ACTOR_LOGF_TRACE("");
  auto ptr = make_counted<C>(std::forward<Ts>(xs)...);
  BOOST_ACTOR_LOGF_DEBUG("spawned actor with ID " << ptr->id());
  BOOST_ACTOR_PUSH_AID(ptr->id());
  if (has_priority_aware_flag(Os)) {
    ptr->is_priority_aware(true);
  }
  if (has_detach_flag(Os) || has_blocking_api_flag(Os)) {
    ptr->is_detached(true);
  }
  before_launch_fun(ptr.get());
  ptr->launch(host, has_lazy_init_flag(Os), has_hide_flag(Os));
  return ptr;
}

/// Called by `spawn` when used to create a class-based actor (usually
/// should not be called by users of the library). This function
/// simply forwards its arguments to `spawn_impl` using `detail::spawn_fwd`.
template <class C, spawn_options Os, typename BeforeLaunch, class... Ts>
intrusive_ptr<C> spawn_class(execution_unit* host,
                             BeforeLaunch before_launch_fun, Ts&&... xs) {
  return spawn_impl<C, Os>(host, before_launch_fun,
                           detail::spawn_fwd<Ts>(xs)...);
}

template <spawn_options Os, class C, class BeforeLaunch, class F, class... Ts>
intrusive_ptr<C> spawn_functor_impl(execution_unit* eu, BeforeLaunch cb,
                                    F fun, Ts&&... xs) {
  constexpr bool has_blocking_base =
    std::is_base_of<blocking_actor, C>::value;
  static_assert(has_blocking_base || ! has_blocking_api_flag(Os),
                "blocking functor-based actors "
                "need to be spawned using the blocking_api flag");
  static_assert(! has_blocking_base || has_blocking_api_flag(Os),
                "non-blocking functor-based actors "
                "cannot be spawned using the blocking_api flag");
  detail::init_fun_factory<C, F> fac;
  auto init = fac(std::move(fun), std::forward<Ts>(xs)...);
  auto bl = [&](C* ptr) {
    cb(ptr);
    ptr->initial_behavior_fac(std::move(init));
  };
  return spawn_class<C, Os>(eu, bl);
}

/// Called by `spawn` when used to create a functor-based actor (usually
/// should not be called by users of the library). This function
/// selects a proper implementation class and then delegates to `spawn_class`.
template <spawn_options Os, typename BeforeLaunch, typename F, class... Ts>
typename infer_handle_from_fun<F>::type
spawn_functor(execution_unit* eu, BeforeLaunch cb, F fun, Ts&&... xs) {
  using impl = typename infer_handle_from_fun<F>::impl;
  return spawn_functor_impl<Os, impl>(eu, cb, std::move(fun),
                                      std::forward<Ts>(xs)...);
}

/// @ingroup ActorCreation
/// @{

/// Returns a new actor of type `C` using `xs...` as constructor
/// arguments. The behavior of `spawn` can be modified by setting `Os`, e.g.,
/// to opt-out of the cooperative scheduling.
template <class C, spawn_options Os = no_spawn_options, class... Ts>
typename infer_handle_from_class<C>::type
spawn(Ts&&... xs) {
  return spawn_class<C, Os>(nullptr, empty_before_launch_callback{},
                            std::forward<Ts>(xs)...);
}

/// Returns a new functor-based actor. The first argument must be the functor,
/// the remainder of `xs...` is used to invoke the functor.
/// The behavior of `spawn` can be modified by setting `Os`, e.g.,
/// to opt-out of the cooperative scheduling.
template <spawn_options Os = no_spawn_options, class F, class... Ts>
typename infer_handle_from_fun<F>::type
spawn(F fun, Ts&&... xs) {
  return spawn_functor<Os>(nullptr, empty_before_launch_callback{},
                           std::move(fun), std::forward<Ts>(xs)...);
}

/// Returns a new actor that immediately, i.e., before this function
/// returns, joins `grps` of type `C` using `xs` as constructor arguments
template <class C, spawn_options Os = no_spawn_options, class Groups,
          class... Ts>
actor spawn_in_groups(const Groups& grps, Ts&&... xs) {
  return spawn_class<C, Os>(nullptr,
                            groups_subscriber<
                              decltype(std::begin(grps))
                            >{std::begin(grps), std::end(grps)},
                            std::forward<Ts>(xs)...);
}

template <class C, spawn_options Os = no_spawn_options, class... Ts>
actor spawn_in_groups(std::initializer_list<group> grps, Ts&&... xs) {
  return spawn_in_groups<
    C, Os, std::initializer_list<group>
  >(grps, std::forward<Ts>(xs)...);
}

/// Returns a new actor that immediately, i.e., before this function
/// returns, joins `grp` of type `C` using `xs` as constructor arguments
template <class C, spawn_options Os = no_spawn_options, class... Ts>
actor spawn_in_group(const group& grp, Ts&&... xs) {
  return spawn_in_groups<C, Os>({grp}, std::forward<Ts>(xs)...);
}

/// Returns a new actor that immediately, i.e., before this function
/// returns, joins `grps`. The first element of `xs` must
/// be the functor, the remaining arguments its arguments.
template <spawn_options Os = no_spawn_options, class Groups, class... Ts>
actor spawn_in_groups(const Groups& grps, Ts&&... xs) {
  static_assert(sizeof...(Ts) > 0, "too few arguments provided");
  return spawn_functor<Os>(nullptr,
                           groups_subscriber<
                             decltype(std::begin(grps))
                           >{std::begin(grps), std::end(grps)},
                           std::forward<Ts>(xs)...);
}

template <spawn_options Os = no_spawn_options, class... Ts>
actor spawn_in_groups(std::initializer_list<group> grps, Ts&&... xs) {
  return spawn_in_groups<
    Os, std::initializer_list<group>
  >(grps, std::forward<Ts>(xs)...);
}

/// Returns a new actor that immediately, i.e., before this function
/// returns, joins `grp`. The first element of `xs` must
/// be the functor, the remaining arguments its arguments.
template <spawn_options Os = no_spawn_options, class... Ts>
actor spawn_in_group(const group& grp, Ts&&... xs) {
  return spawn_in_groups<Os>({grp}, std::forward<Ts>(xs)...);
}

/// Infers the appropriate base class for a functor-based typed actor
/// from the result and the first argument of the functor.
template <class Result,
          class FirstArg,
          bool FirstArgPtr = std::is_pointer<FirstArg>::value
                             && std::is_base_of<
                                  local_actor,
                                  typename std::remove_pointer<FirstArg>::type
                                >::value>
struct BOOST_ACTOR_DEPRECATED infer_typed_actor_base;

template <class... Sigs, class FirstArg>
struct infer_typed_actor_base<typed_behavior<Sigs...>, FirstArg, false> {
  using type = typed_event_based_actor<Sigs...>;
} BOOST_ACTOR_DEPRECATED;

template <class Result, class T>
struct infer_typed_actor_base<Result, T*, true> {
  using type = T;
} BOOST_ACTOR_DEPRECATED;

/// Returns a new typed actor of type `C` using `xs...` as
/// constructor arguments.
template <class C, spawn_options Os = no_spawn_options, class... Ts>
BOOST_ACTOR_DEPRECATED typename actor_handle_from_signature_list<typename C::signatures>::type
spawn_typed(Ts&&... xs) {
  return spawn_class<C, Os>(nullptr, empty_before_launch_callback{},
                            std::forward<Ts>(xs)...);
}

/// Spawns a typed actor from a functor .
template <spawn_options Os, typename BeforeLaunch, typename F, class... Ts>
BOOST_ACTOR_DEPRECATED typename infer_typed_actor_handle<
  typename detail::get_callable_trait<F>::result_type,
  typename detail::tl_head<
    typename detail::get_callable_trait<F>::arg_types
  >::type
>::type
spawn_typed_functor(execution_unit* eu, BeforeLaunch cb, F fun, Ts&&... xs) {
  using impl =
    typename infer_typed_actor_base<
      typename detail::get_callable_trait<F>::result_type,
      typename detail::tl_head<
        typename detail::get_callable_trait<F>::arg_types
      >::type
    >::type;
  return spawn_functor_impl<Os, impl>(eu, cb, std::move(fun),
                                      std::forward<Ts>(xs)...);
}

/// Returns a new typed actor from a functor. The first element
/// of `xs` must be the functor, the remaining arguments are used to
/// invoke the functor. This function delegates its arguments to
/// `spawn_typed_functor`.
template <spawn_options Os = no_spawn_options, typename F, class... Ts>
BOOST_ACTOR_DEPRECATED typename infer_typed_actor_handle<
  typename detail::get_callable_trait<F>::result_type,
  typename detail::tl_head<
    typename detail::get_callable_trait<F>::arg_types
  >::type
>::type
spawn_typed(F fun, Ts&&... xs) {
  return spawn_typed_functor<Os>(nullptr, empty_before_launch_callback{},
                                 std::move(fun), std::forward<Ts>(xs)...);
}

/// @}

} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_SPAWN_HPP
