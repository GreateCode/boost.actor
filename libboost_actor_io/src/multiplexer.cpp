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

#include "boost/actor/io/network/multiplexer.hpp"
#include "boost/actor/io/network/asio_multiplexer.hpp" // default singleton

namespace boost {
namespace actor {
namespace io {
namespace network {

multiplexer::~multiplexer() {
  BOOST_ACTOR_LOG_TRACE("");
}

boost::asio::io_service* pimpl() {
  return nullptr;
}

multiplexer_ptr multiplexer::make() {
  BOOST_ACTOR_LOGF_TRACE("");
  return multiplexer_ptr{new asio_multiplexer};
}

boost::asio::io_service* multiplexer::pimpl() {
  return nullptr;
}

multiplexer::supervisor::~supervisor() {
  // nop
}

multiplexer::runnable::~runnable() {
  // nop
}

} // namespace network
} // namespace io
} // namespace actor
} // namespace boost
