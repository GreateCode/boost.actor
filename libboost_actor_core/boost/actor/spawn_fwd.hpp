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

// this header contains prototype definitions of the spawn function famility;
// implementations can be found in spawn.hpp (this header is included there)

#ifndef BOOST_ACTOR_SPAWN_FWD_HPP
#define BOOST_ACTOR_SPAWN_FWD_HPP

#include "boost/actor/group.hpp"
#include "boost/actor/typed_actor.hpp"
#include "boost/actor/infer_handle.hpp"
#include "boost/actor/spawn_options.hpp"

#include "boost/actor/detail/type_list.hpp"

namespace boost {
namespace actor {

template <class C,
          spawn_options Os = no_spawn_options,
          class BeforeLaunch = std::function<void (C*)>,
          class... Ts>
intrusive_ptr<C> spawn_class(execution_unit* host,
                             BeforeLaunch before_launch_fun,
                             Ts&&... xs);

template <spawn_options Os = no_spawn_options,
          class BeforeLaunch = void (*)(abstract_actor*),
          class F = behavior (*)(),
          class... Ts>
typename infer_handle_from_fun<F>::type
spawn_functor(execution_unit* host,
              BeforeLaunch before_launch_fun,
              F fun,
              Ts&&... xs);

template <class InputIt>
class groups_subscriber {
public:
  inline groups_subscriber(InputIt first, InputIt last)
      : first_(first),
        last_(last) {
    // nop
  }

  template <class T>
  inline void operator()(T* ptr) const {
    for (auto i = first_; i != last_; ++i) {
      ptr->join(*i);
    }
  }

private:
  InputIt first_;
  InputIt last_;
};

struct empty_before_launch_callback {
  template <class T>
  inline void operator()(T*) const {
    // nop
  }
};

/******************************************************************************
 *                      typed actors (deprecated)                             *
 ******************************************************************************/

template <class TypedBehavior, class FirstArg>
struct BOOST_ACTOR_DEPRECATED infer_typed_actor_handle;

// infer actor type from result type if possible
template <class... Sigs, class FirstArg>
struct infer_typed_actor_handle<typed_behavior<Sigs...>, FirstArg> {
  using type = typed_actor<Sigs...>;
} BOOST_ACTOR_DEPRECATED;

// infer actor type from first argument if result type is void
template <class... Sigs>
struct infer_typed_actor_handle<void, typed_event_based_actor<Sigs...>*> {
  using type = typed_actor<Sigs...>;
} BOOST_ACTOR_DEPRECATED;

template <class SignatureList>
struct actor_handle_from_signature_list;

template <class... Sigs>
struct actor_handle_from_signature_list<detail::type_list<Sigs...>> {
  using type = typed_actor<Sigs...>;
} BOOST_ACTOR_DEPRECATED;

template <spawn_options Os, typename BeforeLaunch, typename F, class... Ts>
typename infer_typed_actor_handle<
  typename detail::get_callable_trait<F>::result_type,
  typename detail::tl_head<
    typename detail::get_callable_trait<F>::arg_types
  >::type
>::type
spawn_typed_functor(execution_unit*, BeforeLaunch bl, F fun, Ts&&... xs) BOOST_ACTOR_DEPRECATED;

} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_SPAWN_FWD_HPP
