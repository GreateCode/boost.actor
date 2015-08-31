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

#include "boost/actor/mailbox_element.hpp"

namespace boost {
namespace actor {

mailbox_element::mailbox_element()
    : next(nullptr),
      prev(nullptr),
      marked(false) {
  // nop
}

mailbox_element::mailbox_element(actor_addr arg0, message_id arg1)
    : next(nullptr),
      prev(nullptr),
      marked(false),
      sender(std::move(arg0)),
      mid(arg1) {
  // nop
}

mailbox_element::mailbox_element(actor_addr arg0, message_id arg1, message arg2)
    : next(nullptr),
      prev(nullptr),
      marked(false),
      sender(std::move(arg0)),
      mid(arg1),
      msg(std::move(arg2)) {
  // nop
}

mailbox_element::~mailbox_element() {
  // nop
}

mailbox_element_ptr mailbox_element::make(actor_addr sender, message_id id,
                                            message msg) {
  auto ptr = detail::memory::create<mailbox_element>(std::move(sender), id,
                                                     std::move(msg));
  return mailbox_element_ptr{ptr};
}

} // namespace actor
} // namespace boost
