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

#include "boost/actor/io/doorman.hpp"

#include "boost/actor/detail/logging.hpp"

#include "boost/actor/io/abstract_broker.hpp"

namespace boost {
namespace actor {
namespace io {

doorman::doorman(abstract_broker* ptr, accept_handle acc_hdl, uint16_t p)
    : network::acceptor_manager(ptr),
      hdl_(acc_hdl),
      accept_msg_(make_message(new_connection_msg{hdl_, connection_handle{}})),
      port_(p) {
  // nop
}

doorman::~doorman() {
  // nop
}

void doorman::detach_from_parent() {
  BOOST_ACTOR_LOG_TRACE("hdl = " << hdl().id());
  parent()->doormen_.erase(hdl());
}

message doorman::detach_message() {
  return make_message(acceptor_closed_msg{hdl()});
}

void doorman::io_failure(network::operation op) {
  BOOST_ACTOR_LOG_TRACE("id = " << hdl().id() << ", "
                        << BOOST_ACTOR_TARG(op, static_cast<int>));
  // keep compiler happy when compiling w/o logging
  static_cast<void>(op);
  detach(true);
}

} // namespace io
} // namespace actor
} // namespace boost
