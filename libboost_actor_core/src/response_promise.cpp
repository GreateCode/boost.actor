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

#include <utility>

#include "boost/actor/local_actor.hpp"
#include "boost/actor/response_promise.hpp"

namespace boost {
namespace actor {

response_promise::response_promise(const actor_addr& from, const actor_addr& to,
                                   const message_id& id)
    : from_(from), to_(to), id_(id) {
  BOOST_ACTOR_ASSERT(id.is_response() || ! id.valid());
}

void response_promise::deliver_impl(message msg) const {
  if (! to_)
    return;
  auto to = actor_cast<abstract_actor_ptr>(to_);
  auto from = actor_cast<abstract_actor_ptr>(from_);
  to->enqueue(from_, id_, std::move(msg), from->host());
}

} // namespace actor
} // namespace boost
