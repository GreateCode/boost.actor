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


#ifndef BOOST_ACTOR_IPV4_ACCEPTOR_HPP
#define BOOST_ACTOR_IPV4_ACCEPTOR_HPP

#include <memory>
#include <cstdint>

#include "boost/actor/io/platform.hpp"
#include "boost/actor/io/acceptor.hpp"

namespace boost {
namespace actor {
namespace io {

class ipv4_acceptor : public acceptor {

 public:

    static std::unique_ptr<acceptor> create(std::uint16_t port,
                                            const char* addr = nullptr);

    static std::unique_ptr<acceptor> from_sockfd(native_socket_type fd);

    ~ipv4_acceptor();

    native_socket_type file_handle() const;

    stream_ptr_pair accept_connection();

    optional<stream_ptr_pair> try_accept_connection();

 private:

    ipv4_acceptor(native_socket_type fd, bool nonblocking);

    native_socket_type m_fd;
    bool m_is_nonblocking;

};

} // namespace io
} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_IPV4_ACCEPTOR_HPP
