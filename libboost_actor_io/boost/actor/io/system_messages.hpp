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

#ifndef BOOST_ACTOR_IO_SYSTEM_MESSAGES_HPP
#define BOOST_ACTOR_IO_SYSTEM_MESSAGES_HPP

#include "boost/actor/io/handle.hpp"
#include "boost/actor/io/accept_handle.hpp"
#include "boost/actor/io/connection_handle.hpp"

namespace boost {
namespace actor {
namespace io {

/// Signalizes a newly accepted connection from a {@link broker}.
struct new_connection_msg {
  /// The handle that accepted the new connection.
  accept_handle source;
  /// The handle for the new connection.
  connection_handle handle;
};

/// @relates new_connection_msg
inline bool operator==(const new_connection_msg& lhs,
                       const new_connection_msg& rhs) {
  return lhs.source == rhs.source && lhs.handle == rhs.handle;
}

/// @relates new_connection_msg
inline bool operator!=(const new_connection_msg& lhs,
                       const new_connection_msg& rhs) {
  return !(lhs == rhs);
}

/// Signalizes newly arrived data for a {@link broker}.
struct new_data_msg {
  /// Handle to the related connection.
  connection_handle handle;
  /// Buffer containing the received data.
  std::vector<char> buf;
};

/// @relates new_data_msg
inline bool operator==(const new_data_msg& lhs, const new_data_msg& rhs) {
  return lhs.handle == rhs.handle && lhs.buf == rhs.buf;
}

/// @relates new_data_msg
inline bool operator!=(const new_data_msg& lhs, const new_data_msg& rhs) {
  return !(lhs == rhs);
}

/// Signalizes that a {@link broker} connection has been closed.
struct connection_closed_msg {
  /// Handle to the closed connection.
  connection_handle handle;
};

/// @relates connection_closed_msg
inline bool operator==(const connection_closed_msg& lhs,
                       const connection_closed_msg& rhs) {
  return lhs.handle == rhs.handle;
}

/// @relates connection_closed_msg
inline bool operator!=(const connection_closed_msg& lhs,
                       const connection_closed_msg& rhs) {
  return !(lhs == rhs);
}

/// Signalizes that a {@link broker} acceptor has been closed.
struct acceptor_closed_msg {
  /// Handle to the closed connection.
  accept_handle handle;
};

/// @relates acceptor_closed_msg
inline bool operator==(const acceptor_closed_msg& lhs,
                       const acceptor_closed_msg& rhs) {
  return lhs.handle == rhs.handle;
}

/// @relates acceptor_closed_msg
inline bool operator!=(const acceptor_closed_msg& lhs,
                       const acceptor_closed_msg& rhs) {
  return !(lhs == rhs);
}

} // namespace io
} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_IO_SYSTEM_MESSAGES_HPP
