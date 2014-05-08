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


#ifndef BOOST_ACTOR_BROKER_HPP
#define BOOST_ACTOR_BROKER_HPP

#include <map>

#include "boost/actor/extend.hpp"
#include "boost/actor/local_actor.hpp"

#include "boost/actor/io/stream.hpp"
#include "boost/actor/io/buffer.hpp"
#include "boost/actor/io/acceptor.hpp"
#include "boost/actor/io/input_stream.hpp"
#include "boost/actor/io/output_stream.hpp"
#include "boost/actor/io/accept_handle.hpp"
#include "boost/actor/io/ipv4_acceptor.hpp"
#include "boost/actor/io/ipv4_io_stream.hpp"
#include "boost/actor/io/connection_handle.hpp"

#include "boost/actor/mixin/behavior_stack_based.hpp"

#include "boost/actor/policy/not_prioritizing.hpp"
#include "boost/actor/policy/sequential_invoke.hpp"

namespace boost {
namespace actor {
namespace io {

class broker;

typedef intrusive_ptr<broker> broker_ptr;

broker_ptr init_and_launch(broker_ptr);

/**
 * @brief A broker mediates between a libcppa-based actor system
 *        and other components in the network.
 * @extends local_actor
 */
class broker : public extend<local_actor>::
                      with<mixin::behavior_stack_based<behavior>::impl> {

    friend class policy::sequential_invoke;

    typedef combined_type super;

    // implementation relies on several helper classes ...
    class scribe;
    class servant;
    class doorman;
    class continuation;

    // ... and some helpers need friendship
    friend class scribe;
    friend class doorman;
    friend class continuation;

    friend broker_ptr init_and_launch(broker_ptr);

 public:

    ~broker();

    /**
     * @brief Used to configure {@link receive_policy()}.
     */
    enum policy_flag { at_least, at_most, exactly };

    /**
     * @brief Modifies the receive policy for this broker.
     * @param hdl Identifies the affected connection.
     * @param policy Sets the policy for given buffer size.
     * @param buffer_size Sets the minimal, maximum, or exact number of bytes
     *                    the middleman should read on this connection
     *                    before sending the next {@link new_data_msg}.
     */
    void receive_policy(const connection_handle& hdl,
                        broker::policy_flag policy,
                        size_t buffer_size);

    /**
     * @brief Sends data.
     */
    void write(const connection_handle& hdl, size_t num_bytes, const void* buf);

    /**
     * @brief Sends data.
     */
    void write(const connection_handle& hdl, const buffer& buf);

    /**
     * @brief Sends data.
     */
    void write(const connection_handle& hdl, buffer&& buf);

    /** @cond PRIVATE */

    template<typename F, typename... Ts>
    actor fork(F fun, connection_handle hdl, Ts&&... args) {
        return this->fork_impl(std::bind(std::move(fun),
                                         std::placeholders::_1,
                                         hdl,
                                         std::forward<Ts>(args)...),
                               hdl);
    }

    inline size_t num_connections() const {
        return m_io.size();
    }

    connection_handle add_connection(input_stream_ptr in, output_stream_ptr out);

    inline connection_handle add_connection(stream_ptr sptr) {
        return add_connection(sptr, sptr);
    }

    inline connection_handle add_tcp_connection(native_socket_type tcp_sockfd) {
        return add_connection(ipv4_io_stream::from_sockfd(tcp_sockfd));
    }

    accept_handle add_acceptor(acceptor_uptr ptr);

    inline accept_handle add_tcp_acceptor(native_socket_type tcp_sockfd) {
        return add_acceptor(ipv4_acceptor::from_sockfd(tcp_sockfd));
    }

    void enqueue(msg_hdr_cref, message, execution_unit*) override;

    bool initialized() const;

    template<typename F, typename... Ts>
    static broker_ptr from(F fun,
                           input_stream_ptr in,
                           output_stream_ptr out,
                           Ts&&... args) {
        auto hdl = connection_handle::from_int(in->read_handle());
        return from_impl(std::bind(std::move(fun),
                                   std::placeholders::_1,
                                   hdl,
                                   std::forward<Ts>(args)...),
                         std::move(in),
                         std::move(out));
    }

    static broker_ptr from(std::function<void (broker*)> fun);

    static broker_ptr from(std::function<behavior (broker*)> fun);

    static broker_ptr from(std::function<void (broker*)> fun, acceptor_uptr in);

    static broker_ptr from(std::function<behavior (broker*)> fun, acceptor_uptr in);

    template<typename F, typename T0, typename... Ts>
    static broker_ptr from(F fun, acceptor_uptr in, T0&& arg0, Ts&&... args) {
        return from(std::bind(std::move(fun),
                              std::placeholders::_1,
                              std::forward<T0>(arg0),
                              std::forward<Ts>(args)...),
                    std::move(in));
    }

 protected:

    broker(input_stream_ptr in, output_stream_ptr out);

    broker(acceptor_uptr in);

    broker();

    void cleanup(std::uint32_t reason) override;

    typedef std::unique_ptr<broker::scribe> scribe_pointer;

    typedef std::unique_ptr<broker::doorman> doorman_pointer;

    explicit broker(scribe_pointer);

    virtual behavior make_behavior() = 0;

    /** @endcond */

 private:

    actor fork_impl(std::function<void (broker*)> fun,
                    connection_handle hdl);

    actor fork_impl(std::function<behavior (broker*)> fun,
                    connection_handle hdl);

    static broker_ptr from_impl(std::function<void (broker*)> fun,
                                input_stream_ptr in,
                                output_stream_ptr out);

    static broker_ptr from_impl(std::function<behavior (broker*)> fun,
                                input_stream_ptr in,
                                output_stream_ptr out);

    void invoke_message(msg_hdr_cref hdr, message msg);

    bool invoke_message_from_cache();

    void erase_io(int id);

    void erase_acceptor(int id);

    void init_broker();

    std::map<accept_handle, doorman_pointer> m_accept;
    std::map<connection_handle, scribe_pointer> m_io;

    policy::not_prioritizing  m_priority_policy;
    policy::sequential_invoke m_invoke_policy;

};

} // namespace io
} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_BROKER_HPP
