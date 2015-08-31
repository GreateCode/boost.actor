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

#ifndef BOOST_ACTOR_IO_DOORMAN_HPP
#define BOOST_ACTOR_IO_DOORMAN_HPP

#include <cstddef>

#include "boost/actor/message.hpp"

#include "boost/actor/io/accept_handle.hpp"
#include "boost/actor/io/system_messages.hpp"
#include "boost/actor/io/network/acceptor_manager.hpp"

namespace boost {
namespace actor {
namespace io {

/// Manages incoming connections.
/// @ingroup Broker
class doorman : public network::acceptor_manager {
public:
  doorman(abstract_broker* parent, accept_handle hdl, uint16_t local_port);

  ~doorman();

  inline accept_handle hdl() const {
    return hdl_;
  }

  void io_failure(network::operation op) override;

  // needs to be launched explicitly
  virtual void launch() = 0;

  uint16_t port() const {
    return port_;
  }

protected:
  void detach_from_parent() override;

  message detach_message() override;

  inline new_connection_msg& accept_msg() {
    return accept_msg_.get_as_mutable<new_connection_msg>(0);
  }

  inline const new_connection_msg& accept_msg() const {
    return accept_msg_.get_as<new_connection_msg>(0);
  }

  accept_handle hdl_;
  message accept_msg_;
  uint16_t port_;
};

} // namespace io
} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_IO_DOORMAN_HPP
