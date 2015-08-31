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

#ifndef BOOST_ACTOR_SEND_HPP
#define BOOST_ACTOR_SEND_HPP

#include "boost/actor/actor.hpp"
#include "boost/actor/channel.hpp"
#include "boost/actor/message.hpp"
#include "boost/actor/actor_cast.hpp"
#include "boost/actor/actor_addr.hpp"
#include "boost/actor/message_id.hpp"
#include "boost/actor/message_priority.hpp"
#include "boost/actor/typed_actor.hpp"
#include "boost/actor/system_messages.hpp"
#include "boost/actor/check_typed_input.hpp"

namespace boost {
namespace actor {

/// Sends `to` a message under the identity of `from` with priority `prio`.
template <class... Ts>
void send_as(const actor& from, message_priority prio,
             const channel& to, Ts&&... xs) {
  if (! to) {
    return;
  }
  message_id mid;
  to->enqueue(from.address(),
              prio == message_priority::high ? mid.with_high_priority() : mid,
              make_message(std::forward<Ts>(xs)...), nullptr);
}

/// Sends `to` a message under the identity of `from`.
template <class... Ts>
void send_as(const actor& from, const channel& to, Ts&&... xs) {
  send_as(from, message_priority::normal, to, std::forward<Ts>(xs)...);
}

/// Sends `to` a message under the identity of `from` with priority `prio`.
template <class... Sigs, class... Ts>
void send_as(const actor& from, message_priority prio,
             const typed_actor<Sigs...>& to, Ts&&... xs) {
  using token =
    detail::type_list<
      typename detail::implicit_conversions<
        typename std::decay<Ts>::type
      >::type...>;
  token tk;
  check_typed_input(to, tk);
  send_as(from, prio, actor_cast<channel>(to), std::forward<Ts>(xs)...);
}

/// Sends `to` a message under the identity of `from`.
template <class... Sigs, class... Ts>
void send_as(const actor& from, const typed_actor<Sigs...>& to, Ts&&... xs) {
  using token =
    detail::type_list<
      typename detail::implicit_conversions<
        typename std::decay<Ts>::type
      >::type...>;
  token tk;
  check_typed_input(to, tk);
  send_as(from, message_priority::normal,
          actor_cast<channel>(to), std::forward<Ts>(xs)...);
}

/// Anonymously sends `to` a message with priority `prio`.
template <class... Ts>
void anon_send(message_priority prio, const channel& to, Ts&&... xs) {
  send_as(invalid_actor, prio, to, std::forward<Ts>(xs)...);
}

/// Anonymously sends `to` a message.
template <class... Ts>
void anon_send(const channel& to, Ts&&... xs) {
  send_as(invalid_actor, message_priority::normal, to, std::forward<Ts>(xs)...);
}

/// Anonymously sends `to` a message with priority `prio`.
template <class... Sigs, class... Ts>
void anon_send(message_priority prio, const typed_actor<Sigs...>& to,
               Ts&&... xs) {
  using token =
    detail::type_list<
      typename detail::implicit_conversions<
        typename std::decay<Ts>::type
      >::type...>;
  token tk;
  check_typed_input(to, tk);
  anon_send(prio, actor_cast<channel>(to), std::forward<Ts>(xs)...);
}

/// Anonymously sends `to` a message.
template <class... Sigs, class... Ts>
void anon_send(const typed_actor<Sigs...>& to, Ts&&... xs) {
  using token =
    detail::type_list<
      typename detail::implicit_conversions<
        typename std::decay<Ts>::type
      >::type...>;
  token tk;
  check_typed_input(to, tk);
  anon_send(message_priority::normal, actor_cast<channel>(to),
            std::forward<Ts>(xs)...);
}

/// Anonymously sends `to` an exit message.
inline void anon_send_exit(const actor_addr& to, uint32_t reason) {
  if (! to){
    return;
  }
  auto ptr = actor_cast<actor>(to);
  ptr->enqueue(invalid_actor_addr, message_id{}.with_high_priority(),
               make_message(exit_msg{invalid_actor_addr, reason}), nullptr);
}

/// Anonymously sends `to` an exit message.
template <class ActorHandle>
void anon_send_exit(const ActorHandle& to, uint32_t reason) {
  anon_send_exit(to.address(), reason);
}

} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_SEND_HPP
