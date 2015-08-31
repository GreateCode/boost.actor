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

#include "boost/actor/forwarding_actor_proxy.hpp"

#include "boost/actor/send.hpp"
#include "boost/actor/locks.hpp"
#include "boost/actor/to_string.hpp"

#include "boost/actor/detail/logging.hpp"

namespace boost {
namespace actor {

forwarding_actor_proxy::forwarding_actor_proxy(actor_id aid, node_id nid,
                                               actor mgr)
    : actor_proxy(aid, nid),
      manager_(mgr) {
  BOOST_ACTOR_ASSERT(mgr != invalid_actor);
  BOOST_ACTOR_LOG_INFO(BOOST_ACTOR_ARG(aid) << ", " << BOOST_ACTOR_TARG(nid, to_string));
}

forwarding_actor_proxy::~forwarding_actor_proxy() {
  anon_send(manager_, make_message(delete_atom::value, node(), id()));
}

actor forwarding_actor_proxy::manager() const {
  actor result;
  {
    shared_lock<detail::shared_spinlock> guard_(manager_mtx_);
    result = manager_;
  }
  return result;
}

void forwarding_actor_proxy::manager(actor new_manager) {
  std::unique_lock<detail::shared_spinlock> guard_(manager_mtx_);
  manager_.swap(new_manager);
}

void forwarding_actor_proxy::forward_msg(const actor_addr& sender,
                                         message_id mid, message msg) {
  BOOST_ACTOR_LOG_TRACE(BOOST_ACTOR_ARG(id()) << ", " << BOOST_ACTOR_TSARG(sender) << ", "
                              << BOOST_ACTOR_MARG(mid, integer_value) << ", "
                              << BOOST_ACTOR_TSARG(msg));
  shared_lock<detail::shared_spinlock> guard_(manager_mtx_);
  if (manager_) manager_->enqueue(invalid_actor_addr, invalid_message_id,
                                  make_message(forward_atom::value, sender,
                                               address(), mid, std::move(msg)),
                                  nullptr);
}

void forwarding_actor_proxy::enqueue(const actor_addr& sender, message_id mid,
                                     message m, execution_unit*) {
  forward_msg(sender, mid, std::move(m));
}

bool forwarding_actor_proxy::link_impl(linking_operation op,
                                       const actor_addr& other) {
  switch (op) {
    case establish_link_op:
      if (establish_link_impl(other)) {
        // causes remote actor to link to (proxy of) other
        // receiving peer will call: this->local_link_to(other)
        forward_msg(address(), invalid_message_id,
                    make_message(link_atom::value, other));
        return true;
      }
      break;
    case remove_link_op:
      if (remove_link_impl(other)) {
        // causes remote actor to unlink from (proxy of) other
        forward_msg(address(), invalid_message_id,
                    make_message(unlink_atom::value, other));
        return true;
      }
      break;
    case establish_backlink_op:
      if (establish_backlink_impl(other)) {
        // causes remote actor to unlink from (proxy of) other
        forward_msg(address(), invalid_message_id,
                    make_message(link_atom::value, other));
        return true;
      }
      break;
    case remove_backlink_op:
      if (remove_backlink_impl(other)) {
        // causes remote actor to unlink from (proxy of) other
        forward_msg(address(), invalid_message_id,
                    make_message(unlink_atom::value, other));
        return true;
      }
      break;
  }
  return false;
}

void forwarding_actor_proxy::local_link_to(const actor_addr& other) {
  establish_link_impl(other);
}

void forwarding_actor_proxy::local_unlink_from(const actor_addr& other) {
  remove_link_impl(other);
}

void forwarding_actor_proxy::kill_proxy(uint32_t reason) {
  manager(invalid_actor);
  cleanup(reason);
}

} // namespace actor
} // namespace boost
