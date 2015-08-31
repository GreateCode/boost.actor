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

#include "boost/actor/message_id.hpp"
#include "boost/actor/event_based_actor.hpp"

#include "boost/actor/detail/singletons.hpp"

namespace boost {
namespace actor {

event_based_actor::~event_based_actor() {
  // nop
}

void event_based_actor::forward_to(const actor& whom,
                                   message_priority prio) {
  forward_current_message(whom, prio);
}

void event_based_actor::initialize() {
  is_initialized(true);
  auto bhvr = make_behavior();
  BOOST_ACTOR_LOG_DEBUG_IF(! bhvr, "make_behavior() did not return a behavior, "
                          << "has_behavior() = "
                          << std::boolalpha << this->has_behavior());
  if (bhvr) {
    // make_behavior() did return a behavior instead of using become()
    BOOST_ACTOR_LOG_DEBUG("make_behavior() did return a valid behavior");
    become(std::move(bhvr));
  }
}

behavior event_based_actor::make_behavior() {
  behavior res;
  if (initial_behavior_fac_) {
    res = initial_behavior_fac_(this);
    initial_behavior_fac_ = nullptr;
  }
  return res;
}

} // namespace actor
} // namespace boost
