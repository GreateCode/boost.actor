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

#include "boost/none.hpp"
#include "boost/actor/config.hpp"
#include "boost/actor/make_counted.hpp"

#include "boost/actor/io/broker.hpp"
#include "boost/actor/io/middleman.hpp"

#include "boost/actor/detail/logging.hpp"
#include "boost/actor/detail/singletons.hpp"
#include "boost/actor/detail/scope_guard.hpp"
#include "boost/actor/detail/sync_request_bouncer.hpp"

namespace boost {
namespace actor {
namespace io {

class abstract_broker::continuation {
public:
  continuation(intrusive_ptr<abstract_broker> bptr, mailbox_element_ptr mptr)
      : self_(std::move(bptr)),
        ptr_(std::move(mptr)) {
    // nop
  }

  continuation(continuation&&) = default;

  inline void operator()() {
    BOOST_ACTOR_PUSH_AID(self_->id());
    BOOST_ACTOR_LOG_TRACE("");
    self_->invoke_message(ptr_);
  }

private:
  intrusive_ptr<abstract_broker> self_;
  mailbox_element_ptr ptr_;
};

void abstract_broker::enqueue(mailbox_element_ptr ptr, execution_unit*) {
  backend().post(continuation{this, std::move(ptr)});
}

void abstract_broker::enqueue(const actor_addr& sender, message_id mid,
                              message msg, execution_unit* eu) {
  enqueue(mailbox_element::make(sender, mid, std::move(msg)), eu);
}

void abstract_broker::launch(execution_unit*, bool, bool is_hidden) {
  // add implicit reference count held by the middleman
  ref();
  is_registered(! is_hidden);
  BOOST_ACTOR_PUSH_AID(id());
  BOOST_ACTOR_LOGF_TRACE("init and launch broker with ID " << id());
  // we want to make sure initialization is executed in MM context
  do_become(
    [=](sys_atom) {
      BOOST_ACTOR_LOGF_TRACE("ID " << id());
      bhvr_stack_.pop_back();
      // launch backends now, because user-defined initialization
      // might call functions like add_connection
      for (auto& kvp : doormen_) {
        kvp.second->launch();
      }
      initialize();
    },
    true);
  enqueue(invalid_actor_addr, invalid_message_id,
          make_message(sys_atom::value), nullptr);
}

void abstract_broker::cleanup(uint32_t reason) {
  BOOST_ACTOR_LOG_TRACE(BOOST_ACTOR_ARG(reason));
  planned_exit_reason(reason);
  on_exit();
  close_all();
  BOOST_ACTOR_ASSERT(doormen_.empty());
  BOOST_ACTOR_ASSERT(scribes_.empty());
  cache_.clear();
  local_actor::cleanup(reason);
  deref(); // release implicit reference count from middleman
}

abstract_broker::~abstract_broker() {
  // nop
}

void abstract_broker::configure_read(connection_handle hdl,
                                     receive_policy::config cfg) {
  BOOST_ACTOR_LOG_TRACE(BOOST_ACTOR_MARG(hdl, id) << ", cfg = {" << static_cast<int>(cfg.first)
                                  << ", " << cfg.second << "}");
  by_id(hdl).configure_read(cfg);
}

std::vector<char>& abstract_broker::wr_buf(connection_handle hdl) {
  return by_id(hdl).wr_buf();
}

void abstract_broker::write(connection_handle hdl, size_t bs, const void* buf) {
  auto& out = wr_buf(hdl);
  auto first = reinterpret_cast<const char*>(buf);
  auto last = first + bs;
  out.insert(out.end(), first, last);
}

void abstract_broker::flush(connection_handle hdl) {
  by_id(hdl).flush();
}

std::vector<connection_handle> abstract_broker::connections() const {
  std::vector<connection_handle> result;
  result.reserve(scribes_.size());
  for (auto& kvp : scribes_) {
    result.push_back(kvp.first);
  }
  return result;
}

void abstract_broker::add_scribe(const intrusive_ptr<scribe>& ptr) {
  scribes_.emplace(ptr->hdl(), ptr);
}
connection_handle abstract_broker::add_tcp_scribe(const std::string& hostname,
                                                  uint16_t port) {
  BOOST_ACTOR_LOG_TRACE(BOOST_ACTOR_ARG(hostname) << ", " << BOOST_ACTOR_ARG(port));
  return backend().add_tcp_scribe(this, hostname, port);
}

void abstract_broker::assign_tcp_scribe(connection_handle hdl) {
  BOOST_ACTOR_LOG_TRACE(BOOST_ACTOR_MARG(hdl, id));
  backend().assign_tcp_scribe(this, hdl);
}

connection_handle
abstract_broker::add_tcp_scribe(network::native_socket fd) {
  BOOST_ACTOR_LOG_TRACE(BOOST_ACTOR_ARG(fd));
  return backend().add_tcp_scribe(this, fd);
}

void abstract_broker::add_doorman(const intrusive_ptr<doorman>& ptr) {
  doormen_.emplace(ptr->hdl(), ptr);
  if (is_initialized())
    ptr->launch();
}

std::pair<accept_handle, uint16_t>
abstract_broker::add_tcp_doorman(uint16_t port, const char* in,
                                 bool reuse_addr) {
  BOOST_ACTOR_LOG_TRACE(BOOST_ACTOR_ARG(port) << ", in = " << (in ? in : "nullptr")
                << ", " << BOOST_ACTOR_ARG(reuse_addr));
  return backend().add_tcp_doorman(this, port, in, reuse_addr);
}

void abstract_broker::assign_tcp_doorman(accept_handle hdl) {
  BOOST_ACTOR_LOG_TRACE(BOOST_ACTOR_MARG(hdl, id));
  backend().assign_tcp_doorman(this, hdl);
}

accept_handle abstract_broker::add_tcp_doorman(network::native_socket fd) {
  BOOST_ACTOR_LOG_TRACE(BOOST_ACTOR_ARG(fd));
  return backend().add_tcp_doorman(this, fd);
}

uint16_t abstract_broker::local_port(accept_handle hdl) {
  auto i = doormen_.find(hdl);
  return i != doormen_.end() ? i->second->port() : 0;
}

optional<accept_handle> abstract_broker::hdl_by_port(uint16_t port) {
  for (auto& kvp : doormen_)
    if (kvp.second->port() == port)
      return kvp.first;
  return none;
}

void abstract_broker::invoke_message(mailbox_element_ptr& ptr) {
  BOOST_ACTOR_LOG_TRACE(BOOST_ACTOR_TARG(ptr->msg, to_string));
  if (exit_reason() != exit_reason::not_exited || ! has_behavior()) {
    BOOST_ACTOR_LOG_DEBUG("actor already finished execution"
                  << ", planned_exit_reason = " << planned_exit_reason()
                  << ", has_behavior() = " << has_behavior());
    if (ptr->mid.valid()) {
      detail::sync_request_bouncer srb{exit_reason()};
      srb(ptr->sender, ptr->mid);
    }
    return;
  }
  // prepare actor for invocation of message handler
  try {
    auto& bhvr = this->awaits_response()
                 ? this->awaited_response_handler()
                 : this->bhvr_stack().back();
    auto bid = this->awaited_response_id();
    switch (local_actor::invoke_message(ptr, bhvr, bid)) {
      case im_success: {
        BOOST_ACTOR_LOG_DEBUG("handle_message returned hm_msg_handled");
        while (has_behavior()
               && planned_exit_reason() == exit_reason::not_exited
               && invoke_message_from_cache()) {
          // rinse and repeat
        }
        break;
      }
      case im_dropped:
        BOOST_ACTOR_LOG_DEBUG("handle_message returned hm_drop_msg");
        break;
      case im_skipped: {
        BOOST_ACTOR_LOG_DEBUG("handle_message returned hm_skip_msg or hm_cache_msg");
        if (ptr) {
          cache_.push_second_back(ptr.release());
        }
        break;
      }
    }
  }
  catch (std::exception& e) {
    BOOST_ACTOR_LOG_INFO("broker killed due to an unhandled exception: "
                 << to_verbose_string(e));
    // keep compiler happy in non-debug mode
    static_cast<void>(e);
    quit(exit_reason::unhandled_exception);
  }
  catch (...) {
    BOOST_ACTOR_LOG_ERROR("broker killed due to an unknown exception");
    quit(exit_reason::unhandled_exception);
  }
  // safe to actually release behaviors now
  bhvr_stack().cleanup();
  // cleanup actor if needed
  if (planned_exit_reason() != exit_reason::not_exited) {
    cleanup(planned_exit_reason());
  } else if (! has_behavior()) {
    BOOST_ACTOR_LOG_DEBUG("no behavior set, quit for normal exit reason");
    quit(exit_reason::normal);
    cleanup(planned_exit_reason());
  }
}

void abstract_broker::invoke_message(const actor_addr& sender,
                                     message_id mid,
                                     message& msg) {
  auto ptr = mailbox_element::make(sender, mid, message{});
  ptr->msg.swap(msg);
  invoke_message(ptr);
  if (ptr)
    ptr->msg.swap(msg);
}

void abstract_broker::close_all() {
  BOOST_ACTOR_LOG_TRACE("");
  while (! doormen_.empty()) {
    // stop_reading will remove the doorman from doormen_
    doormen_.begin()->second->stop_reading();
  }
  while (! scribes_.empty()) {
    // stop_reading will remove the scribe from scribes_
    scribes_.begin()->second->stop_reading();
  }
}

abstract_broker::abstract_broker() : mm_(*middleman::instance()) {
  // nop
}

abstract_broker::abstract_broker(middleman& ptr) : mm_(ptr) {
  // nop
}

network::multiplexer& abstract_broker::backend() {
  return mm_.backend();
}

bool abstract_broker::invoke_message_from_cache() {
  BOOST_ACTOR_LOG_TRACE("");
  auto& bhvr = this->awaits_response()
               ? this->awaited_response_handler()
               : this->bhvr_stack().back();
  auto bid = this->awaited_response_id();
  auto i = cache_.second_begin();
  auto e = cache_.second_end();
  BOOST_ACTOR_LOG_DEBUG(std::distance(i, e) << " elements in cache");
  return cache_.invoke(static_cast<local_actor*>(this), i, e, bhvr, bid);
}

} // namespace io
} // namespace actor
} // namespace boost
