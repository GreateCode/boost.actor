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

#ifndef BOOST_ACTOR_IO_NETWORK_ACCEPTOR_MANAGER_HPP
#define BOOST_ACTOR_IO_NETWORK_ACCEPTOR_MANAGER_HPP

#include "boost/actor/io/network/manager.hpp"

namespace boost {
namespace actor {
namespace io {
namespace network {

/// An acceptor manager configures an acceptor and provides
/// callbacks for incoming connections as well as for error handling.
class acceptor_manager : public manager {
public:
  acceptor_manager(abstract_broker* ptr);

  ~acceptor_manager();

  /// Called by the underlying IO device to indicate that
  /// a new connection is awaiting acceptance.
  virtual void new_connection() = 0;
};

} // namespace network
} // namespace io
} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_IO_NETWORK_ACCEPTOR_MANAGER_HPP
