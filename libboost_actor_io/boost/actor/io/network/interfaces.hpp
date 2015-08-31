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

#ifndef BOOST_ACTOR_IO_NETWORK_INTERFACES_HPP
#define BOOST_ACTOR_IO_NETWORK_INTERFACES_HPP

#include <map>
#include <vector>
#include <string>
#include <utility>

#include "boost/optional.hpp"

#include "boost/actor/io/network/protocol.hpp"

namespace boost {
namespace actor {
namespace io {
namespace network {

// {protocol => address}
using address_listing = std::map<protocol, std::vector<std::string>>;

// {interface_name => {protocol => address}}
using interfaces_map = std::map<std::string, address_listing>;

/// Utility class bundling access to network interface names and addresses.
class interfaces {
public:
  /// Returns a map listing each interface by its name.
  static interfaces_map list_all(bool include_localhost = true);

  /// Returns all addresses for all devices for all protocols.
  static address_listing list_addresses(bool include_localhost = true);

  /// Returns all addresses for all devices for given protocol.
  static std::vector<std::string> list_addresses(protocol proc,
                                                 bool include_localhost = true);

  /// Returns a native IPv4 or IPv6 translation of `host`.
  ///*/
  static optional<std::pair<std::string, protocol>>
  native_address(const std::string& host, optional<protocol> preferred = none);
};

} // namespace network
} // namespace io
} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_IO_NETWORK_INTERFACES_HPP
