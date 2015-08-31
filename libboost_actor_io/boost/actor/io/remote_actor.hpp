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

#ifndef BOOST_ACTOR_IO_REMOTE_ACTOR_HPP
#define BOOST_ACTOR_IO_REMOTE_ACTOR_HPP

#include <set>
#include <string>
#include <cstdint>

#include "boost/actor/fwd.hpp"
#include "boost/actor/actor_cast.hpp"
#include "boost/actor/typed_actor.hpp"

namespace boost {
namespace actor {
namespace io {

actor_addr remote_actor_impl(std::set<std::string> ifs,
                             std::string host, uint16_t port);

/// Establish a new connection to the actor at `host` on given `port`.
/// @param host Valid hostname or IP address.
/// @param port TCP port.
/// @returns An {@link actor_ptr} to the proxy instance
///          representing a remote actor.
/// @throws network_error Thrown on connection error or
///                       when connecting to a typed actor.
inline actor remote_actor(std::string host, uint16_t port) {
  auto res = remote_actor_impl(std::set<std::string>{}, std::move(host), port);
  return actor_cast<actor>(res);
}

/// Establish a new connection to the typed actor at `host` on given `port`.
/// @param host Valid hostname or IP address.
/// @param port TCP port.
/// @returns An {@link actor_ptr} to the proxy instance
///          representing a typed remote actor.
/// @throws network_error Thrown on connection error or when connecting
///                       to an untyped otherwise unexpected actor.
template <class ActorHandle>
ActorHandle typed_remote_actor(std::string host, uint16_t port) {
  auto res = remote_actor_impl(ActorHandle::message_types(),
                               std::move(host), port);
  return actor_cast<ActorHandle>(res);
}

} // namespace io
} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_IO_REMOTE_ACTOR_HPP
