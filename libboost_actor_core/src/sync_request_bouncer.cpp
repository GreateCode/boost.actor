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

#include "boost/actor/atom.hpp"
#include "boost/actor/config.hpp"
#include "boost/actor/actor_cast.hpp"
#include "boost/actor/message_id.hpp"
#include "boost/actor/exit_reason.hpp"
#include "boost/actor/mailbox_element.hpp"
#include "boost/actor/system_messages.hpp"

#include "boost/actor/detail/sync_request_bouncer.hpp"

namespace boost {
namespace actor {
namespace detail {

sync_request_bouncer::sync_request_bouncer(uint32_t r)
    : rsn(r == exit_reason::not_exited ? exit_reason::normal : r) {
  // nop
}

void sync_request_bouncer::operator()(const actor_addr& sender,
                                      const message_id& mid) const {
  BOOST_ACTOR_ASSERT(rsn != exit_reason::not_exited);
  if (sender && mid.is_request()) {
    auto ptr = actor_cast<abstract_actor_ptr>(sender);
    ptr->enqueue(invalid_actor_addr, mid.response_id(),
                 make_message(sync_exited_msg{sender, rsn}),
                 // TODO: this breaks out of the execution unit
                 nullptr);
  }
}

void sync_request_bouncer::operator()(const mailbox_element& e) const {
  (*this)(e.sender, e.mid);
}

} // namespace detail
} // namespace actor
} // namespace boost
