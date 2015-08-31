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

#ifndef BOOST_ACTOR_ACTOR_COMPANION_HPP
#define BOOST_ACTOR_ACTOR_COMPANION_HPP

#include <memory>
#include <functional>

#include "boost/actor/local_actor.hpp"
#include "boost/actor/mailbox_element.hpp"
#include "boost/actor/abstract_event_based_actor.hpp"

#include "boost/actor/mixin/sync_sender.hpp"

#include "boost/actor/detail/disposer.hpp"
#include "boost/actor/detail/shared_spinlock.hpp"

namespace boost {
namespace actor {

/// An co-existing forwarding all messages through a user-defined
/// callback to another object, thus serving as gateway to
/// allow any object to interact with other actors.
/// @extends local_actor
class actor_companion : public abstract_event_based_actor<behavior, true> {
public:
  using lock_type = detail::shared_spinlock;
  using message_pointer = std::unique_ptr<mailbox_element, detail::disposer>;
  using enqueue_handler = std::function<void (message_pointer)>;

  /// Removes the handler for incoming messages and terminates
  /// the companion for exit reason @ rsn.
  void disconnect(std::uint32_t rsn = exit_reason::normal);

  /// Sets the handler for incoming messages.
  /// @warning `handler` needs to be thread-safe
  void on_enqueue(enqueue_handler handler);

  void enqueue(mailbox_element_ptr what, execution_unit* host) override;

  void enqueue(const actor_addr& sender, message_id mid,
               message content, execution_unit* host) override;

  void initialize() override;

private:
  // set by parent to define custom enqueue action
  enqueue_handler on_enqueue_;

  // guards access to handler_
  lock_type lock_;
};

/// A pointer to a co-existing (actor) object.
/// @relates actor_companion
using actor_companion_ptr = intrusive_ptr<actor_companion>;

} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_ACTOR_COMPANION_HPP
