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

#include "boost/actor/io/scribe.hpp"

#include "boost/actor/detail/logging.hpp"

#include "boost/actor/io/abstract_broker.hpp"

namespace boost {
namespace actor {
namespace io {

scribe::scribe(abstract_broker* ptr, connection_handle conn_hdl)
    : network::stream_manager(ptr),
      hdl_(conn_hdl) {
  std::vector<char> tmp;
  read_msg_ = make_message(new_data_msg{hdl_, std::move(tmp)});
}

void scribe::detach_from_parent() {
  BOOST_ACTOR_LOG_TRACE("hdl = " << hdl().id());
  parent()->scribes_.erase(hdl());
}

scribe::~scribe() {
  BOOST_ACTOR_LOG_TRACE("");
}

message scribe::detach_message() {
  return make_message(connection_closed_msg{hdl()});
}

void scribe::consume(const void*, size_t num_bytes) {
  BOOST_ACTOR_LOG_TRACE(BOOST_ACTOR_ARG(num_bytes));
  if (detached()) {
    // we are already disconnected from the broker while the multiplexer
    // did not yet remove the socket, this can happen if an IO event causes
    // the broker to call close_all() while the pollset contained
    // further activities for the broker
    return;
  }
  auto& buf = rd_buf();
  BOOST_ACTOR_ASSERT(buf.size() >= num_bytes);
  // make sure size is correct, swap into message, and then call client
  buf.resize(num_bytes);
  read_msg().buf.swap(buf);
  parent()->invoke_message(invalid_actor_addr, invalid_message_id, read_msg_);
  if (! read_msg_.empty()) {
    // swap buffer back to stream and implicitly flush wr_buf()
    read_msg().buf.swap(buf);
    flush();
  }
}

void scribe::io_failure(network::operation op) {
  BOOST_ACTOR_LOG_TRACE("id = " << hdl().id()
                << ", " << BOOST_ACTOR_TARG(op, static_cast<int>));
  // keep compiler happy when compiling w/o logging
  static_cast<void>(op);
  detach(true);
}

} // namespace io
} // namespace actor
} // namespace boost
