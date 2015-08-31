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

#include "boost/actor/exception.hpp"
#include "boost/actor/blocking_actor.hpp"

#include "boost/actor/detail/logging.hpp"
#include "boost/actor/detail/singletons.hpp"
#include "boost/actor/detail/actor_registry.hpp"

namespace boost {
namespace actor {

blocking_actor::blocking_actor() {
  is_blocking(true);
}

blocking_actor::~blocking_actor() {
  // avoid weak-vtables warning
}

void blocking_actor::await_all_other_actors_done() {
  detail::singletons::get_actor_registry()->await_running_count_equal(1);
}

void blocking_actor::act() {
  BOOST_ACTOR_LOG_TRACE("");
  if (initial_behavior_fac_)
    initial_behavior_fac_(this);
}

void blocking_actor::initialize() {
  // nop
}

void blocking_actor::dequeue(behavior& bhvr, message_id mid) {
  BOOST_ACTOR_LOG_TRACE(BOOST_ACTOR_MARG(mid, integer_value));
  // try to dequeue from cache first
  if (invoke_from_cache(bhvr, mid)) {
    return;
  }
  // requesting an invalid timeout will reset our active timeout
  uint32_t timeout_id = 0;
  if (mid == invalid_message_id) {
    timeout_id = request_timeout(bhvr.timeout());
  } else {
    request_sync_timeout_msg(bhvr.timeout(), mid);
  }
  // read incoming messages
  for (;;) {
    await_data();
    auto msg = next_message();
    switch (invoke_message(msg, bhvr, mid)) {
      case im_success:
        if (mid == invalid_message_id) {
          reset_timeout(timeout_id);
        }
        return;
      case im_skipped:
        if (msg) {
          push_to_cache(std::move(msg));
        }
        break;
      default:
        // delete msg
        break;
    }
  }
}

} // namespace actor
} // namespace boost
