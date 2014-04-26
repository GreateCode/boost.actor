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


#include <iostream>

#include "boost/none.hpp"

#include "boost/actor/config.hpp"

#include "boost/actor/logging.hpp"
#include "boost/actor/singletons.hpp"

#include "boost/actor/detail/scope_guard.hpp"

#include "boost/actor/io/broker.hpp"
#include "boost/actor/io/broker.hpp"
#include "boost/actor/io/middleman.hpp"
#include "boost/actor/io/buffered_writing.hpp"

#include "boost/actor/detail/make_counted.hpp"
#include "boost/actor/detail/actor_registry.hpp"
#include "boost/actor/detail/sync_request_bouncer.hpp"

using std::cout;
using std::endl;
using std::move;

namespace boost {
namespace actor {
namespace io {

namespace {

constexpr size_t default_max_buffer_size = 65535;

} // namespace <anonymous>

default_broker::default_broker(function_type f,
                               input_stream_ptr in,
                               output_stream_ptr out)
    : broker(std::move(in), std::move(out)), m_fun(std::move(f)) { }

default_broker::default_broker(function_type f, scribe_pointer ptr)
    : broker(std::move(ptr)), m_fun(std::move(f)) { }

default_broker::default_broker(function_type f, acceptor_uptr ptr)
    : broker(std::move(ptr)), m_fun(std::move(f)) { }

behavior default_broker::make_behavior() {
    BOOST_ACTOR_PUSH_AID(id());
    BOOST_ACTOR_LOG_TRACE("");
    enqueue({invalid_actor_addr, channel{this}},
            make_message(atom("INITMSG")),
            nullptr);
    return (
        on(atom("INITMSG")) >> [=] {
            unbecome();
            auto bhvr = m_fun(this);
            if (bhvr) become(std::move(bhvr));
        }
    );
}

class broker::continuation {

 public:

    continuation(broker_ptr ptr, msg_hdr_cref hdr, message&& msg)
    : m_self(move(ptr)), m_hdr(hdr), m_data(move(msg)) { }

    inline void operator()() {
        BOOST_ACTOR_PUSH_AID(m_self->id());
        BOOST_ACTOR_LOG_TRACE("");
        m_self->invoke_message(m_hdr, move(m_data));
    }

 private:

    broker_ptr     m_self;
    message_header m_hdr;
    message      m_data;

};

class broker::servant : public continuable {

    typedef continuable super;

 public:

    ~servant();

    template<typename... Ts>
    servant(broker_ptr parent, Ts&&... args)
    : super{std::forward<Ts>(args)...}, m_disconnected{false}
    , m_broker{move(parent)} { }

    void io_failed(event_bitmask mask) override {
        if (mask == event::read) disconnect();
    }

    void dispose() override {
        auto ptr = m_broker;
        ptr->erase_io(read_handle());
        if (ptr->m_io.empty() && ptr->m_accept.empty()) {
            // release implicit reference count held by middleman
            // in caes no reader/writer is left for this broker
            ptr->deref();
        }
    }

    void set_broker(broker_ptr new_broker) {
        if (!m_disconnected) m_broker = std::move(new_broker);
    }

 protected:

    void disconnect() {
        if (!m_disconnected) {
            m_disconnected = true;
            if (m_broker->exit_reason() == exit_reason::not_exited) {
                m_broker->invoke_message({invalid_actor_addr, invalid_actor},
                                         disconnect_message());
            }
        }
    }

    virtual message disconnect_message() = 0;

    bool m_disconnected;

    broker_ptr m_broker;

};

// avoid weak-vtables warning by providing dtor out-of-line
broker::servant::~servant() { }

class broker::scribe : public extend<broker::servant>::with<buffered_writing> {

    typedef combined_type super;

 public:

    ~scribe();

    scribe(broker_ptr parent, input_stream_ptr in, output_stream_ptr out)
    : super{get_middleman(), out, move(parent), in->read_handle(), out->write_handle()}
    , m_is_continue_reading{false}, m_dirty{false}
    , m_policy{broker::at_least}, m_policy_buffer_size{0}, m_in{in}
    , m_read_msg(make_message(new_data_msg{})) {
        auto& ndm = read_msg();
        ndm.handle = connection_handle::from_int(in->read_handle());
        ndm.buf.final_size(default_max_buffer_size);
    }

    void receive_policy(broker::policy_flag policy, size_t buffer_size) {
        BOOST_ACTOR_LOG_TRACE(BOOST_ACTOR_ARG(policy) << ", " << BOOST_ACTOR_ARG(buffer_size));
        if (not m_disconnected) {
            m_dirty = true;
            m_policy = policy;
            m_policy_buffer_size = buffer_size;
        }
    }

    continue_reading_result continue_reading() override {
        BOOST_ACTOR_LOG_TRACE("");
        m_is_continue_reading = true;
        auto sg = detail::make_scope_guard([=] {
            m_is_continue_reading = false;
        });
        for (;;) {
            // stop reading if actor finished execution
            if (m_broker->exit_reason() != exit_reason::not_exited) {
                BOOST_ACTOR_LOG_DEBUG("broker already done; exit reason: "
                               << m_broker->exit_reason());
                return continue_reading_result::closed;
            }
            auto& buf = read_msg().buf;
            if (m_dirty) {
                m_dirty = false;
                if (m_policy == broker::at_most || m_policy == broker::exactly) {
                    buf.final_size(m_policy_buffer_size);
                }
                else buf.final_size(default_max_buffer_size);
            }
            auto before = buf.size();
            try { buf.append_from(m_in.get()); }
            catch (std::ios_base::failure&) {
                disconnect();
                return continue_reading_result::failure;
            }
            BOOST_ACTOR_LOG_DEBUG("received " << (buf.size() - before) << " bytes");
            if  ( before == buf.size()
               || (m_policy == broker::exactly && buf.size() != m_policy_buffer_size)) {
                return continue_reading_result::continue_later;
            }
            if  ( (   m_policy == broker::at_least
                   && buf.size() >= m_policy_buffer_size)
               || m_policy == broker::exactly
               || m_policy == broker::at_most) {
                BOOST_ACTOR_LOG_DEBUG("invoke io actor");
                m_broker->invoke_message({invalid_actor_addr, nullptr}, m_read_msg);
                BOOST_ACTOR_LOG_INFO_IF(!m_read_msg.vals()->unique(),
                                        "client detached buffer");
                read_msg().buf.clear();
            }
        }
    }

    connection_handle id() const {
        return connection_handle::from_int(m_in->read_handle());
    }

 protected:

    message disconnect_message() override {
        auto hdl = connection_handle::from_int(m_in->read_handle());
        return make_message(connection_closed_msg{hdl});
    }

 private:

    new_data_msg& read_msg() {
        return m_read_msg.get_as_mutable<new_data_msg>(0);
    }

    bool m_is_continue_reading;
    bool m_dirty;
    broker::policy_flag m_policy;
    size_t m_policy_buffer_size;
    input_stream_ptr m_in;
    message m_read_msg;

};

// avoid weak-vtables warning by providing dtor out-of-line
broker::scribe::~scribe() { }

class broker::doorman : public broker::servant {

    typedef servant super;

 public:

    ~doorman();

    doorman(broker_ptr parent, acceptor_uptr ptr)
            : super{move(parent), ptr->file_handle()}
            , m_accept_msg(make_message(new_connection_msg{})) {
        accept_msg().source = accept_handle::from_int(ptr->file_handle());
        m_ptr.swap(ptr);
    }

    continue_reading_result continue_reading() override {
        BOOST_ACTOR_LOG_TRACE("");
        for (;;) {
            optional<stream_ptr_pair> opt{none};
            try { opt = m_ptr->try_accept_connection(); }
            catch (std::exception& e) {
                BOOST_ACTOR_LOG_ERROR(to_verbose_string(e));
                static_cast<void>(e); // keep compiler happy
                return continue_reading_result::failure;
            }
            if (opt) {
                using namespace std;
                auto& p = *opt;
                accept_msg().handle = m_broker->add_scribe(move(p.first),
                                                           move(p.second));
                m_broker->invoke_message({invalid_actor_addr, nullptr}, m_accept_msg);
            }
            else return continue_reading_result::continue_later;
       }
    }

 protected:

    message disconnect_message() override {
        auto hdl = accept_handle::from_int(m_ptr->file_handle());
        return make_message(acceptor_closed_msg{hdl});
    }

 private:

    new_connection_msg& accept_msg() {
        return m_accept_msg.get_as_mutable<new_connection_msg>(0);
    }

    acceptor_uptr m_ptr;
    message m_accept_msg;

};

// avoid weak-vtables warning by providing dtor out-of-line
broker::doorman::~doorman() { }

void broker::invoke_message(msg_hdr_cref hdr, message msg) {
    BOOST_ACTOR_LOG_TRACE(BOOST_ACTOR_TARG(msg, to_string));
    if (planned_exit_reason() != exit_reason::not_exited || bhvr_stack().empty()) {
        BOOST_ACTOR_LOG_DEBUG("actor already finished execution"
                       << ", planned_exit_reason = " << planned_exit_reason()
                       << ", bhvr_stack().empty() = " << bhvr_stack().empty());
        if (hdr.id.valid()) {
            detail::sync_request_bouncer srb{exit_reason()};
            srb(hdr.sender, hdr.id);
        }
        return;
    }
    // prepare actor for invocation of message handler
    m_dummy_node.sender = hdr.sender;
    m_dummy_node.msg = move(msg);
    m_dummy_node.mid = hdr.id;
    try {
        auto bhvr = bhvr_stack().back();
        auto mid = bhvr_stack().back_id();
        switch (m_invoke_policy.handle_message(this, &m_dummy_node, bhvr, mid)) {
            case policy::hm_msg_handled: {
                BOOST_ACTOR_LOG_DEBUG("handle_message returned hm_msg_handled");
                while (   !bhvr_stack().empty()
                       && planned_exit_reason() == exit_reason::not_exited
                       && invoke_message_from_cache()) {
                    // rinse and repeat
                }
                break;
            }
            case policy::hm_drop_msg:
                BOOST_ACTOR_LOG_DEBUG("handle_message returned hm_drop_msg");
                break;
            case policy::hm_skip_msg:
            case policy::hm_cache_msg: {
                BOOST_ACTOR_LOG_DEBUG("handle_message returned hm_skip_msg or hm_cache_msg");
                auto e = mailbox_element::create(hdr, move(m_dummy_node.msg));
                m_priority_policy.push_to_cache(unique_mailbox_element_pointer{e});
                break;
            }
        }
    }
    catch (std::exception& e) {
        BOOST_ACTOR_LOG_ERROR("broker killed due to an unhandled exception: "
                       << to_verbose_string(e));
        // keep compiler happy in non-debug mode
        static_cast<void>(e);
        quit(exit_reason::unhandled_exception);
    }
    catch (...) {
        BOOST_ACTOR_LOG_ERROR("broker killed due to an unhandled exception");
        quit(exit_reason::unhandled_exception);
    }
    // restore dummy node
    m_dummy_node.sender = actor_addr{};
    m_dummy_node.msg.reset();
    // cleanup if needed
    if (planned_exit_reason() != exit_reason::not_exited) {
        cleanup(planned_exit_reason());
    }
    else if (bhvr_stack().empty()) {
        BOOST_ACTOR_LOG_DEBUG("bhvr_stack().empty(), quit for normal exit reason");
        quit(exit_reason::normal);
        cleanup(planned_exit_reason());
    }
}

bool broker::invoke_message_from_cache() {
    BOOST_ACTOR_LOG_TRACE("");
    auto bhvr = bhvr_stack().back();
    auto mid = bhvr_stack().back_id();
    auto e = m_priority_policy.cache_end();
    BOOST_ACTOR_LOG_DEBUG(std::distance(m_priority_policy.cache_begin(), e)
                   << " elements in cache");
    for (auto i = m_priority_policy.cache_begin(); i != e; ++i) {
        auto res = m_invoke_policy.invoke_message(this, *i, bhvr, mid);
        if (res || !*i) {
            m_priority_policy.cache_erase(i);
            if (res) return true;
            return invoke_message_from_cache();
        }
    }
    return false;
}

void broker::enqueue(msg_hdr_cref hdr, message msg, execution_unit*) {
    get_middleman()->run_later(continuation{this, hdr, move(msg)});
}

bool broker::initialized() const {
    return true;
}

void broker::init_broker() {
    // acquire implicit reference count held by the middleman
    ref();
    // actor is running now
    get_actor_registry()->inc_running();
}

broker::broker(input_stream_ptr in, output_stream_ptr out) {
    using namespace std;
    init_broker();
    add_scribe(move(in), move(out));
}

broker::broker(scribe_pointer ptr) {
    using namespace std;
    init_broker();
    auto id = ptr->id();
    m_io.insert(make_pair(id, std::move(ptr)));
}

broker::broker(acceptor_uptr ptr) {
    using namespace std;
    init_broker();
    add_doorman(std::move(ptr));
}

void broker::cleanup(std::uint32_t reason) {
    super::cleanup(reason);
    get_actor_registry()->dec_running();
}

void broker::write(const connection_handle& hdl, size_t num_bytes, const void* buf) {
    auto i = m_io.find(hdl);
    if (i != m_io.end()) i->second->write(num_bytes, buf);
}

void broker::write(const connection_handle& hdl, const io::buffer& buf) {
    write(hdl, buf.size(), buf.data());
}

void broker::write(const connection_handle& hdl, io::buffer&& buf) {
    write(hdl, buf.size(), buf.data());
    buf.clear();
}

broker_ptr init_and_launch(broker_ptr ptr) {
    BOOST_ACTOR_PUSH_AID(ptr->id());
    BOOST_ACTOR_LOGF_TRACE("init and launch actor with id " << ptr->id());
    // continue reader only if not inherited from default_broker_impl
    auto mm = get_middleman();
    mm->run_later([=] {
        BOOST_ACTOR_LOGC_TRACE("NONE", "init_and_launch::run_later_functor", "");
        BOOST_ACTOR_LOGF_WARNING_IF(ptr->m_io.empty() && ptr->m_accept.empty(),
                             "both m_io and m_accept are empty");
        // 'launch' all backends
        BOOST_ACTOR_LOGC_DEBUG("NONE", "init_and_launch::run_later_functor",
                        "add " << ptr->m_io.size() << " IO servants");
        for (auto& kvp : ptr->m_io)
            mm->continue_reader(kvp.second.get());
        BOOST_ACTOR_LOGC_DEBUG("NONE", "init_and_launch::run_later_functor",
                        "add " << ptr->m_accept.size() << " acceptors");
        for (auto& kvp : ptr->m_accept)
            mm->continue_reader(kvp.second.get());
    });
    // exec initialization code
    auto bhvr = ptr->make_behavior();
    if (bhvr) ptr->become(std::move(bhvr));
    BOOST_ACTOR_LOGF_WARNING_IF(!ptr->has_behavior(), "broker w/o behavior spawned");
    return ptr;
}

broker_ptr broker::from_impl(std::function<behavior (broker*)> fun,
                             input_stream_ptr in,
                             output_stream_ptr out) {
    return detail::make_counted<default_broker>(std::move(fun),
                                                std::move(in),
                                                std::move(out));
}


broker_ptr broker::from_impl(std::function<void (broker*)> fun,
                             input_stream_ptr in,
                             output_stream_ptr out) {
    auto f = [=](broker* ptr) -> behavior { fun(ptr); return behavior{}; };
    return detail::make_counted<default_broker>(f, std::move(in), std::move(out));
}

broker_ptr broker::from(std::function<behavior (broker*)> fun, acceptor_uptr in) {
    return detail::make_counted<default_broker>(std::move(fun), std::move(in));
}


broker_ptr broker::from(std::function<void (broker*)> fun, acceptor_uptr in) {
    auto f = [=](broker* ptr) -> behavior { fun(ptr); return behavior{}; };
    return detail::make_counted<default_broker>(f, std::move(in));
}

void broker::erase_io(int id) {
    m_io.erase(connection_handle::from_int(id));
}

void broker::erase_acceptor(int id) {
    m_accept.erase(accept_handle::from_int(id));
}

connection_handle broker::add_scribe(input_stream_ptr in, output_stream_ptr out) {
    using namespace std;
    auto id = connection_handle::from_int(in->read_handle());
    m_io.insert(make_pair(id, scribe_pointer{new scribe(this, move(in), move(out))}));
    return id;
}

accept_handle broker::add_doorman(acceptor_uptr ptr) {
    using namespace std;
    auto id = accept_handle::from_int(ptr->file_handle());
    m_accept.insert(make_pair(id, doorman_pointer{new doorman(this, std::move(ptr))}));
    return id;
}

actor broker::fork_impl(std::function<void (broker*)> fun,
                        connection_handle hdl) {
    BOOST_ACTOR_LOG_TRACE(BOOST_ACTOR_MARG(hdl, id));
    auto i = m_io.find(hdl);
    if (i == m_io.end()) {
        BOOST_ACTOR_LOG_ERROR("invalid handle");
        throw std::invalid_argument("invalid handle");
    }
    scribe* sptr = i->second.get(); // non-owning pointer
    auto f = [=](broker* ptr) -> behavior { fun(ptr); return behavior{}; };
    auto result = detail::make_counted<default_broker>(f, std::move(i->second));
    init_and_launch(result);
    sptr->set_broker(result); // set new broker
    m_io.erase(i);
    return {result};
}

void broker::receive_policy(const connection_handle& hdl,
                            broker::policy_flag policy,
                            size_t buffer_size) {
    auto i = m_io.find(hdl);
    if (i != m_io.end()) i->second->receive_policy(policy, buffer_size);
}

broker::~broker() {
    BOOST_ACTOR_LOG_TRACE("");
}

} // namespace io
} // namespace actor
} // namespace boost
