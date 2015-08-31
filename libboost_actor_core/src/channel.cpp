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

#include "boost/actor/actor.hpp"
#include "boost/actor/group.hpp"
#include "boost/actor/channel.hpp"
#include "boost/actor/message.hpp"
#include "boost/actor/actor_cast.hpp"

namespace boost {
namespace actor {

channel::channel(const actor& other)
    : ptr_(actor_cast<abstract_channel_ptr>(other)) {
  // nop
}

channel::channel(const group& other) : ptr_(other.ptr()) {
  // nop
}

channel::channel(const invalid_actor_t&) : ptr_(nullptr) {
  // nop
}

channel::channel(const invalid_group_t&) : ptr_(nullptr) {
  // nop
}

intptr_t channel::compare(const abstract_channel* lhs,
                          const abstract_channel* rhs) {
  return reinterpret_cast<intptr_t>(lhs) - reinterpret_cast<intptr_t>(rhs);
}

channel::channel(abstract_channel* ptr) : ptr_(ptr) {}

intptr_t channel::compare(const channel& other) const {
  return compare(ptr_.get(), other.ptr_.get());
}

intptr_t channel::compare(const actor& other) const {
  return compare(ptr_.get(), actor_cast<abstract_actor_ptr>(other).get());
}

intptr_t channel::compare(const abstract_channel* other) const {
  return compare(ptr_.get(), other);
}

} // namespace actor
} // namespace boost
