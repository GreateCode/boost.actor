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

#ifndef BOOST_ACTOR_IO_PUBLISH_HPP
#define BOOST_ACTOR_IO_PUBLISH_HPP

#include <set>
#include <string>
#include <cstdint>

#include "boost/actor/actor.hpp"
#include "boost/actor/actor_cast.hpp"
#include "boost/actor/typed_actor.hpp"

namespace boost {
namespace actor {
namespace io {

uint16_t publish_impl(uint16_t port, actor_addr whom,
                      std::set<std::string> sigs, const char* in,
                      bool reuse_addr);

/// Publishes `whom` at `port`. The connection is managed by the middleman.
/// @param whom Actor that should be published at `port`.
/// @param port Unused TCP port.
/// @param in The IP address to listen to or `INADDR_ANY` if `in == nullptr`.
/// @returns The actual port the OS uses after `bind()`. If `port == 0` the OS
///          chooses a random high-level port.
/// @throws bind_failure
inline uint16_t publish(boost::actor::actor whom, uint16_t port,
                        const char* in = nullptr, bool reuse_addr = false) {
  return ! whom ? 0 : publish_impl(port, whom->address(),
                                   std::set<std::string>{}, in, reuse_addr);
}

/// @copydoc publish(actor,uint16_t,const char*)
template <class... Sigs>
uint16_t typed_publish(typed_actor<Sigs...> whom, uint16_t port,
                       const char* in = nullptr, bool reuse_addr = false) {
  return ! whom ? 0 : publish_impl(port, whom->address(), whom.message_types(),
                                   in, reuse_addr);
}

} // namespace io
} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_IO_PUBLISH_HPP
