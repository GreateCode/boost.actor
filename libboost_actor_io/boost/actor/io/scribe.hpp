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

#ifndef BOOST_ACTOR_IO_SCRIBE_HPP
#define BOOST_ACTOR_IO_SCRIBE_HPP

#include <vector>

#include "boost/actor/message.hpp"

#include "boost/actor/io/receive_policy.hpp"
#include "boost/actor/io/system_messages.hpp"
#include "boost/actor/io/network/stream_manager.hpp"

namespace boost {
namespace actor {
namespace io {

/// Manages a stream.
/// @ingroup Broker
class scribe : public network::stream_manager {
public:
  scribe(abstract_broker* parent, connection_handle hdl);

  ~scribe();

  /// Implicitly starts the read loop on first call.
  virtual void configure_read(receive_policy::config config) = 0;

  /// Grants access to the output buffer.
  virtual std::vector<char>& wr_buf() = 0;

  /// Flushes the output buffer, i.e., sends the content of
  ///    the buffer via the network.
  virtual void flush() = 0;

  inline connection_handle hdl() const {
    return hdl_;
  }

  void io_failure(network::operation op) override;

  void consume(const void* data, size_t num_bytes) override;

protected:
  virtual std::vector<char>& rd_buf() = 0;

  inline new_data_msg& read_msg() {
    return read_msg_.get_as_mutable<new_data_msg>(0);
  }

  inline const new_data_msg& read_msg() const {
    return read_msg_.get_as<new_data_msg>(0);
  }

  void detach_from_parent() override;

  message detach_message() override;

private:
  connection_handle hdl_;
  message read_msg_;
};

} // namespace io
} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_IO_SCRIBE_HPP
