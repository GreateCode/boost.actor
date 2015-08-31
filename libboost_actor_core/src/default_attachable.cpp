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

#include "boost/actor/default_attachable.hpp"

#include "boost/actor/message.hpp"
#include "boost/actor/system_messages.hpp"
#include "boost/actor/actor_cast.hpp"

namespace boost {
namespace actor {

namespace {

template <class MsgType>
message make(abstract_actor* self, uint32_t reason) {
  return make_message(MsgType{self->address(), reason});
}

} // namespace <anonymous>

void default_attachable::actor_exited(abstract_actor* self, uint32_t reason) {
  BOOST_ACTOR_ASSERT(self->address() != observer_);
  auto factory = type_ == monitor ? &make<down_msg> : &make<exit_msg>;
  auto ptr = actor_cast<abstract_actor_ptr>(observer_);
  ptr->enqueue(self->address(), message_id{}.with_high_priority(),
               factory(self, reason), self->host());
}

bool default_attachable::matches(const token& what) {
  if (what.subtype != attachable::token::observer) {
    return false;
  }
  auto& ot = *reinterpret_cast<const observe_token*>(what.ptr);
  return ot.observer == observer_ && ot.type == type_;
}

default_attachable::default_attachable(actor_addr observer, observe_type type)
    : observer_(std::move(observer)),
      type_(type) {
  // nop
}

} // namespace actor
} // namespace boost
