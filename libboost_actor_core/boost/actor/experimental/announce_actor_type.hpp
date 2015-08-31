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

#ifndef BOOST_ACTOR_EXPERIMENTAL_ANNOUNCE_ACTOR_HPP
#define BOOST_ACTOR_EXPERIMENTAL_ANNOUNCE_ACTOR_HPP

#include <type_traits>

#include "boost/actor/actor.hpp"
#include "boost/actor/actor_addr.hpp"
#include "boost/actor/infer_handle.hpp"

#include "boost/actor/detail/singletons.hpp"
#include "boost/actor/detail/type_traits.hpp"
#include "boost/actor/detail/actor_registry.hpp"

namespace boost {
namespace actor {
namespace experimental {

using spawn_result = std::pair<actor_addr, std::set<std::string>>;

using spawn_fun = std::function<spawn_result (message)>;

using selfptr_mode_token = spawn_mode_token<spawn_mode::function_with_selfptr>;

using void_mode_token = spawn_mode_token<spawn_mode::function>;

template <class T>
void dyn_spawn_prepare_message(message&, T*, void_mode_token) {
  // nop
}

template <class T>
void dyn_spawn_prepare_message(message& msg, T* self, selfptr_mode_token) {
  using std::swap;
  message tmp;
  swap(msg, tmp);
  // we can't use make_message here because of the implicit conversions
  using storage = detail::tuple_vals<T*>;
  auto ptr = make_counted<storage>(self);
  msg = message{detail::message_data::cow_ptr{std::move(ptr)}} + tmp;
}

template <class F>
spawn_fun make_spawn_fun(F fun) {
  return [fun](message msg) -> spawn_result {
    using trait = infer_handle_from_fun<F>;
    using impl = typename trait::impl;
    using behavior_t = typename trait::behavior_type;
    spawn_mode_token<trait::mode> tk;
    auto ptr = make_counted<impl>();
    dyn_spawn_prepare_message<impl>(msg, ptr.get(), tk);
    ptr->initial_behavior_fac([=](local_actor*) -> behavior {
      auto res = const_cast<message&>(msg).apply(fun);
      if (res && res->size() > 0 && res->template match_element<behavior_t>(0)) 
        return std::move(res->template get_as_mutable<behavior_t>(0).unbox());
      return {};
    });
    ptr->launch(nullptr, false, false);
    return {ptr->address(), trait::type::message_types()};
  };
}

template <class T, class... Ts>
spawn_result dyn_spawn_class(message msg) {
  using handle = typename infer_handle_from_class<T>::type;
  using pointer = intrusive_ptr<T>;
  pointer ptr;
  auto factory = &make_counted<T, Ts...>;
  auto res = msg.apply(factory);
  if (!res || res->empty() || !res->template match_element<pointer>(0))
    return {};
  ptr = std::move(res->template get_as_mutable<pointer>(0));
  ptr->launch(nullptr, false, false);
  return {ptr->address(), handle::message_types()};
}

template <class T, class... Ts>
spawn_fun make_spawn_fun() {
  static_assert(std::is_same<T*, decltype(new T(std::declval<Ts>()...))>::value,
                "no constructor for T(Ts...) exists");
  static_assert(detail::conjunction<
                  std::is_lvalue_reference<Ts>::value...
                >::value,
                "all Ts must be lvalue references");
  static_assert(std::is_base_of<local_actor, T>::value,
                "T is not derived from local_actor");
  return &dyn_spawn_class<T, Ts...>;
}

actor spawn_announce_actor_type_server();

void announce_actor_type_impl(std::string&& name, spawn_fun f);

template <class F>
void announce_actor_type(std::string name, F fun) {
  announce_actor_type_impl(std::move(name), make_spawn_fun(fun));
}

template <class T, class... Ts>
void announce_actor_type(std::string name) {
  announce_actor_type_impl(std::move(name), make_spawn_fun<T, Ts...>());
}

} // namespace experimental
} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_EXPERIMENTAL_ANNOUNCE_ACTOR_HPP
