/******************************************************************************\
 *           ___        __                                                    *
 *          /\_ \    __/\ \                                                   *
 *          \//\ \  /\_\ \ \____    ___   _____   _____      __               *
 *            \ \ \ \/\ \ \ '__`\  /'___\/\ '__`\/\ '__`\  /'__`\             *
 *             \_\ \_\ \ \ \ \L\ \/\ \__/\ \ \L\ \ \ \L\ \/\ \L\.\_           *
 *             /\____\\ \_\ \_,__/\ \____\\ \ ,__/\ \ ,__/\ \__/.\_\          *
 *             \/____/ \/_/\/___/  \/____/ \ \ \/  \ \ \/  \/__/\/_/          *
 *                                          \ \_\   \ \_\                     *
 *                                           \/_/    \/_/                     *
 *                                                                            *
 * Copyright (C) 2011-2013                                                    *
 * Dominik Charousset <dominik.charousset@haw-hamburg.de>                     *
 *                                                                            *
 * This file is part of libcppa.                                              *
 * libcppa is free software: you can redistribute it and/or modify it under   *
 * the terms of the GNU Lesser General Public License as published by the     *
 * Free Software Foundation; either version 2.1 of the License,               *
 * or (at your option) any later version.                                     *
 *                                                                            *
 * libcppa is distributed in the hope that it will be useful,                 *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                       *
 * See the GNU Lesser General Public License for more details.                *
 *                                                                            *
 * You should have received a copy of the GNU Lesser General Public License   *
 * along with libcppa. If not, see <http://www.gnu.org/licenses/>.            *
\******************************************************************************/


#ifndef FD_UTIL_HPP
#define FD_UTIL_HPP

#include <string>
#include <utility>  // std::pair
#include <unistd.h> // ssize_t

#include "boost/actor/io/platform.hpp"

namespace boost {
namespace actor {
namespace io {
namespace fd_util {

std::string last_socket_error_as_string();

// throws ios_base::failure and adds errno failure if @p add_errno_failure
void throw_io_failure [[noreturn]] (const char* what, bool add_errno = true);

// sets fd to nonblocking if <tt>set_nonblocking == true</tt>
// or to blocking if <tt>set_nonblocking == false</tt>
// throws @p ios_base::failure on error
void nonblocking(native_socket_type fd, bool new_value);

// returns true if fd is nodelay socket
// throws @p ios_base::failure on error
void tcp_nodelay(native_socket_type fd, bool new_value);

// reads @p result and @p errno and throws @p ios_base::failure on error
void handle_write_result(ssize_t result, bool is_nonblocking_io);

// reads @p result and @p errno and throws @p ios_base::failure on error
void handle_read_result(ssize_t result, bool is_nonblocking_io);

std::pair<native_socket_type, native_socket_type> create_pipe();

} // namespace fd_util
} // namespace io
} // namespace actor
} // namespace boost

#endif // FD_UTIL_HPP
