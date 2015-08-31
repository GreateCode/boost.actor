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

#include "boost/actor/scoped_actor.hpp"

#include "boost/actor/spawn_options.hpp"

#include "boost/actor/detail/singletons.hpp"
#include "boost/actor/detail/actor_registry.hpp"

namespace boost {
namespace actor {

namespace {

struct impl : blocking_actor {
  impl() {
    is_detached(true);
  }

  void act() override {
    BOOST_ACTOR_LOG_ERROR("act() of scoped_actor impl called");
  }
};

} // namespace <anonymous>

void scoped_actor::init(bool hide_actor) {
  self_.reset(new impl, false);
  if (! hide_actor) {
    prev_ = BOOST_ACTOR_SET_AID(self_->id());
  }
  BOOST_ACTOR_LOG_TRACE(BOOST_ACTOR_ARG(hide_actor));
  self_->is_registered(! hide_actor);
}

scoped_actor::scoped_actor() {
  init(false);
}

scoped_actor::scoped_actor(bool hide_actor) {
  init(hide_actor);
}

scoped_actor::~scoped_actor() {
  BOOST_ACTOR_LOG_TRACE("");
  if (self_->is_registered()) {
    BOOST_ACTOR_SET_AID(prev_);
  }
  auto r = self_->planned_exit_reason();
  self_->cleanup(r == exit_reason::not_exited ? exit_reason::normal : r);
}

} // namespace actor
} // namespace boost
