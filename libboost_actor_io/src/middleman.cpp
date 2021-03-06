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

#include <tuple>
#include <cerrno>
#include <memory>
#include <cstring>
#include <sstream>
#include <stdexcept>

#include "boost/actor/on.hpp"
#include "boost/actor/actor.hpp"
#include "boost/actor/config.hpp"
#include "boost/actor/node_id.hpp"
#include "boost/actor/announce.hpp"
#include "boost/actor/exception.hpp"
#include "boost/actor/to_string.hpp"
#include "boost/actor/actor_proxy.hpp"
#include "boost/actor/make_counted.hpp"
#include "boost/actor/scoped_actor.hpp"
#include "boost/actor/uniform_type_info.hpp"

#include "boost/actor/io/middleman.hpp"
#include "boost/actor/io/basp_broker.hpp"
#include "boost/actor/io/system_messages.hpp"

#include "boost/actor/io/network/interfaces.hpp"

#include "boost/actor/detail/logging.hpp"
#include "boost/actor/detail/ripemd_160.hpp"
#include "boost/actor/detail/safe_equal.hpp"
#include "boost/actor/detail/singletons.hpp"
#include "boost/actor/detail/get_root_uuid.hpp"
#include "boost/actor/detail/actor_registry.hpp"
#include "boost/actor/detail/get_mac_addresses.hpp"

#ifdef BOOST_ACTOR_WINDOWS
# include <io.h>
# include <fcntl.h>
#endif

namespace boost {
namespace actor {
namespace io {

namespace {

void serialize(serializer& sink, std::vector<char>& buf) {
  sink << static_cast<uint32_t>(buf.size());
  sink.write_raw(buf.size(), buf.data());
}

void serialize(deserializer& source, std::vector<char>& buf) {
  auto bs = source.read<uint32_t>();
  buf.resize(bs);
  source.read_raw(buf.size(), buf.data());
}

template <class Subtype, class I>
void serialize(serializer& sink, handle<Subtype, I>& hdl) {
  sink.write_value(hdl.id());
}

template <class Subtype, class I>
void serialize(deserializer& source, handle<Subtype, I>& hdl) {
  hdl.set_id(source.read<int64_t>());
}

template <class Archive>
void serialize(Archive& ar, new_connection_msg& msg) {
  serialize(ar, msg.source);
  serialize(ar, msg.handle);
}

template <class Archive>
void serialize(Archive& ar, new_data_msg& msg) {
  serialize(ar, msg.handle);
  serialize(ar, msg.buf);
}


// connection_closed_msg & acceptor_closed_msg have the same fields
template <class Archive, class T>
typename std::enable_if<
  std::is_same<T, connection_closed_msg>::value
  || std::is_same<T, acceptor_closed_msg>::value
>::type
serialize(Archive& ar, T& dm) {
  serialize(ar, dm.handle);
}

template <class T>
class uti_impl : public abstract_uniform_type_info<T> {
public:
  using super = abstract_uniform_type_info<T>;

  explicit uti_impl(const char* tname) : super(tname) {
    // nop
  }

  void serialize(const void* instance, serializer* sink) const {
    io::serialize(*sink, super::deref(const_cast<void*>(instance)));
  }

  void deserialize(void* instance, deserializer* source) const {
    io::serialize(*source, super::deref(instance));
  }
};

template <class T>
void do_announce(const char* tname) {
  announce(typeid(T), uniform_type_info_ptr{new uti_impl<T>(tname)});
}

class middleman_actor_impl : public middleman_actor::base {
public:
  middleman_actor_impl(middleman& mref, actor default_broker)
      : broker_(default_broker),
        parent_(mref) {
    // nop
  }

  void on_exit() {
    BOOST_ACTOR_LOG_TRACE("");
    broker_ = invalid_actor;
  }

  using put_res = either<ok_atom, uint16_t>::or_else<error_atom, std::string>;

  using get_res = delegated<either<ok_atom, node_id, actor_addr,
                                   std::set<std::string>>
                            ::or_else<error_atom, std::string>>;

  using del_res = delegated<either<ok_atom>::or_else<error_atom, std::string>>;

  behavior_type make_behavior() override {
    return {
      [=](publish_atom, uint16_t port, actor_addr& whom,
          std::set<std::string>& sigs, std::string& addr, bool reuse) {
        return put(port, whom, sigs, addr.c_str(), reuse);
      },
      [=](open_atom, uint16_t port, std::string& addr, bool reuse) -> put_res {
        actor_addr whom = invalid_actor_addr;
        std::set<std::string> sigs;
        return put(port, whom, sigs, addr.c_str(), reuse);
      },
      [=](connect_atom, const std::string& hostname, uint16_t port) -> get_res {
        BOOST_ACTOR_LOG_TRACE(BOOST_ACTOR_ARG(hostname) << ", " << BOOST_ACTOR_ARG(port));
        try {
          auto hdl = parent_.backend().new_tcp_scribe(hostname, port);
          delegate(broker_, connect_atom::value, hdl, port);
        }
        catch (network_error& err) {
          // fullfil promise immediately
          std::string msg = "network_error: ";
          msg += err.what();
          auto rp = make_response_promise();
          rp.deliver(make_message(error_atom::value, std::move(msg)));
        }
        return {};
      },
      [=](unpublish_atom, const actor_addr&, uint16_t) -> del_res {
        forward_current_message(broker_);
        return {};
      },
      [=](close_atom, uint16_t) -> del_res {
        forward_current_message(broker_);
        return {};
      },
      [=](spawn_atom, const node_id&, const std::string&, const message&)
      -> delegated<either<ok_atom, actor_addr, std::set<std::string>>
                   ::or_else<error_atom, std::string>> {
        forward_current_message(broker_);
        return {};
      }
    };
  }

private:
  put_res put(uint16_t port, actor_addr& whom,
              std::set<std::string>& sigs, const char* in = nullptr,
              bool reuse_addr = false) {
    BOOST_ACTOR_LOG_TRACE(BOOST_ACTOR_TSARG(whom) << ", " << BOOST_ACTOR_ARG(port) << ", "
                                  << BOOST_ACTOR_ARG(reuse_addr));
    accept_handle hdl;
    uint16_t actual_port;
    try {
      // treat empty strings like nullptr
      if (in != nullptr && in[0] == '\0') {
        in = nullptr;
      }
      auto res = parent_.backend().new_tcp_doorman(port, in, reuse_addr);
      hdl = res.first;
      actual_port = res.second;
    }
    catch (bind_failure& err) {
      return {error_atom::value, std::string("bind_failure: ") + err.what()};
    }
    catch (network_error& err) {
      return {error_atom::value, std::string("network_error: ") + err.what()};
    }
    send(broker_, publish_atom::value, hdl, actual_port,
         std::move(whom), std::move(sigs));
    return {ok_atom::value, actual_port};
  }

  actor broker_;
  middleman& parent_;
};

} // namespace <anonymous>

middleman* middleman::instance() {
  BOOST_ACTOR_LOGF_TRACE("");
  // store lambda in a plain old function pointer to make sure
  // std::function has minimal overhead
  using funptr = backend_pointer (*)();
  funptr backend_fac = [] { return network::multiplexer::make(); };
  auto fac = [&] { return new middleman(backend_fac); };
  auto sid = detail::singletons::middleman_plugin_id;
  auto res = detail::singletons::get_plugin_singleton(sid, fac);
  return static_cast<middleman*>(res);
}

void middleman::add_broker(broker_ptr bptr) {
  brokers_.insert(bptr);
  bptr->attach_functor([=](uint32_t) { brokers_.erase(bptr); });
}

void middleman::initialize() {
  BOOST_ACTOR_LOG_TRACE("");
  backend_supervisor_ = backend_->make_supervisor();
  if (backend_supervisor_ == nullptr) {
    // the only backend that returns a `nullptr` is the `test_multiplexer`
    // which does not have its own thread but uses the main thread instead
    backend_->thread_id(std::this_thread::get_id());
  } else {
    thread_ = std::thread{[this] {
      BOOST_ACTOR_LOG_TRACE("");
      backend_->run();
    }};
    backend_->thread_id(thread_.get_id());
  }
  // announce io-related types
  announce<network::protocol>("boost::actor::io::network::protocol");
  announce<network::address_listing>("boost::actor::io::network::address_listing");
  do_announce<new_data_msg>("boost::actor::io::new_data_msg");
  do_announce<new_connection_msg>("boost::actor::io::new_connection_msg");
  do_announce<acceptor_closed_msg>("boost::actor::io::acceptor_closed_msg");
  do_announce<connection_closed_msg>("boost::actor::io::connection_closed_msg");
  do_announce<accept_handle>("boost::actor::io::accept_handle");
  do_announce<acceptor_closed_msg>("boost::actor::io::acceptor_closed_msg");
  do_announce<connection_closed_msg>("boost::actor::io::connection_closed_msg");
  do_announce<connection_handle>("boost::actor::io::connection_handle");
  do_announce<new_connection_msg>("boost::actor::io::new_connection_msg");
  do_announce<new_data_msg>("boost::actor::io::new_data_msg");
  actor mgr = get_named_broker<basp_broker>(atom("_BASP"));
  manager_ = spawn<middleman_actor_impl, detached + hidden>(*this, mgr);
}

void middleman::stop() {
  BOOST_ACTOR_LOG_TRACE("");
  backend_->dispatch([=] {
    BOOST_ACTOR_LOG_TRACE("");
    notify<hook::before_shutdown>();
    // managers_ will be modified while we are stopping each manager,
    // because each manager will call remove(...)
    for (auto& kvp : named_brokers_) {
      if (kvp.second->exit_reason() == exit_reason::not_exited) {
        kvp.second->cleanup(exit_reason::normal);
      }
    }
  });
  backend_supervisor_.reset();
  if (thread_.joinable())
    thread_.join();
  hooks_.reset();
  named_brokers_.clear();
  scoped_actor self(true);
  self->monitor(manager_);
  self->send_exit(manager_, exit_reason::user_shutdown);
  self->receive(
    [](const down_msg&) {
      // nop
    }
  );
}

void middleman::dispose() {
  delete this;
}

middleman::middleman(const backend_factory& factory) : backend_(factory()) {
  // nop
}

middleman::~middleman() {
  // nop
}

middleman_actor middleman::actor_handle() {
  return manager_;
}

middleman_actor get_middleman_actor() {
  return middleman::instance()->actor_handle();
}

} // namespace io
} // namespace actor
} // namespace boost
