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

#include "boost/actor/abstract_channel.hpp"

#include "boost/actor/mailbox_element.hpp"
#include "boost/actor/detail/singletons.hpp"

namespace boost {
namespace actor {

using detail::singletons;

abstract_channel::abstract_channel(int init_flags)
    : flags_(init_flags),
      node_(singletons::get_node_id()) {
  // nop
}

abstract_channel::abstract_channel(int init_flags, node_id nid)
    : flags_(init_flags),
      node_(std::move(nid)) {
  // nop
}

abstract_channel::~abstract_channel() {
  // nop
}

void abstract_channel::enqueue(mailbox_element_ptr what, execution_unit* host) {
  enqueue(what->sender, what->mid, std::move(what->msg), host);
}

bool abstract_channel::is_remote() const {
  return node_ != singletons::get_node_id();
}

} // namespace actor
} // namespace boost
