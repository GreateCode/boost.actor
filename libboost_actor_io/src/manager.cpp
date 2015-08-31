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

#include "boost/actor/io/network/manager.hpp"

#include "boost/actor/detail/logging.hpp"

#include "boost/actor/io/abstract_broker.hpp"

namespace boost {
namespace actor {
namespace io {
namespace network {

manager::manager(abstract_broker* ptr) : parent_(ptr) {
  // nop
}

manager::~manager() {
  // nop
}

void manager::set_parent(abstract_broker* ptr) {
  if (! detached())
    parent_ = ptr;
}

void manager::detach(bool invoke_disconnect_message) {
  BOOST_ACTOR_LOG_TRACE("");
  if (! detached()) {
    BOOST_ACTOR_LOG_DEBUG("disconnect servant from broker");
    detach_from_parent();
    if (invoke_disconnect_message) {
      auto msg = detach_message();
      parent_->invoke_message(parent_->address(),invalid_message_id, msg);
    }
    parent_ = nullptr;
  }
}

} // namespace network
} // namespace io
} // namespace actor
} // namespace boost
