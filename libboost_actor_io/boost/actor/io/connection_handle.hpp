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

#ifndef BOOST_ACTOR_IO_CONNECTION_HANDLE_HPP
#define BOOST_ACTOR_IO_CONNECTION_HANDLE_HPP

#include <functional>

#include "boost/actor/io/handle.hpp"

namespace boost {
namespace actor {
namespace io {

struct invalid_connection_handle_t {
  constexpr invalid_connection_handle_t() {
    // nop
  }
};

constexpr invalid_connection_handle_t invalid_connection_handle
  = invalid_connection_handle_t{};

/// Generic handle type for identifying connections.
class connection_handle : public handle<connection_handle,
                                        invalid_connection_handle_t> {
public:
  friend class handle<connection_handle, invalid_connection_handle_t>;

  using super = handle<connection_handle, invalid_connection_handle_t>;

  constexpr connection_handle() {
    // nop
  }

  constexpr connection_handle(const invalid_connection_handle_t&) {
    // nop
  }

 private:
  inline connection_handle(int64_t handle_id) : super(handle_id) {
    // nop
  }
};

} // namespace io
} // namespace actor
} // namespace boost

namespace std{

template<>
struct hash<boost::actor::io::connection_handle> {
  size_t operator()(const boost::actor::io::connection_handle& hdl) const {
    hash<int64_t> f;
    return f(hdl.id());
  }
};

} // namespace std

#endif // BOOST_ACTOR_IO_CONNECTION_HANDLE_HPP
