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
 * Raphael Hiesgen <raphael.hiesgen (at) haw-hamburg.de>                      *
 *                                                                            *
 * Distributed under the terms and conditions of the BSD 3-Clause License or  *
 * (at your option) under the terms and conditions of the Boost Software      *
 * License 1.0. See accompanying files LICENSE and LICENSE_ALTERNATIVE.       *
 *                                                                            *
 * If you did not receive a copy of the license files, see                    *
 * http://opensource.org/licenses/BSD-3-Clause and                            *
 * http://www.boost.org/LICENSE_1_0.txt.                                      *
 ******************************************************************************/

#include "boost/actor/exception.hpp"

#include "boost/actor/io/broker.hpp"
#include "boost/actor/io/middleman.hpp"

#include "boost/actor/io/network/asio_multiplexer.hpp"

namespace boost {
namespace actor {
namespace io {
namespace network {

namespace {

/// A wrapper for the supervisor backend provided by boost::asio.
struct asio_supervisor : public multiplexer::supervisor {
  explicit asio_supervisor(io_backend& iob) : work(iob) {
    // nop
  }

private:
  boost::asio::io_service::work work;
};

} // namespace anonymous

default_socket new_tcp_connection(io_backend& backend, const std::string& host,
                                  uint16_t port) {
  default_socket fd{backend};
  using boost::asio::ip::tcp;
  tcp::resolver r(fd.get_io_service());
  tcp::resolver::query q(host, std::to_string(port));
  boost::system::error_code ec;
  auto i = r.resolve(q, ec);
  if (ec) {
    throw network_error("could not resolve host: " + host);
  }
  boost::asio::connect(fd, i, ec);
  if (ec) {
    throw network_error("could not connect to host: " + host +
                        ":" + std::to_string(port));
  }
  return fd;
}

void ip_bind(default_socket_acceptor& fd, uint16_t port,
             const char* addr = nullptr, bool reuse_addr = true) {
  BOOST_ACTOR_LOGF_TRACE(BOOST_ACTOR_ARG(port));
  using boost::asio::ip::tcp;
  try {
    auto bind_and_listen = [&](tcp::endpoint& ep) {
      fd.open(ep.protocol());
      fd.set_option(tcp::acceptor::reuse_address(reuse_addr));
      fd.bind(ep);
      fd.listen();
    };
    if (addr) {
      tcp::endpoint ep(boost::asio::ip::address::from_string(addr), port);
      bind_and_listen(ep);
      BOOST_ACTOR_LOGF_DEBUG("created IPv6 endpoint: " << ep.address() << ":"
                                               << fd.local_endpoint().port());
    } else {
      tcp::endpoint ep(tcp::v6(), port);
      bind_and_listen(ep);
      BOOST_ACTOR_LOGF_DEBUG("created IPv6 endpoint: " << ep.address() << ":"
                                               << fd.local_endpoint().port());
    }
  } catch (boost::system::system_error& se) {
    if (se.code() == boost::system::errc::address_in_use) {
      throw bind_failure(se.code().message());
    }
    throw network_error(se.code().message());
  }
}

connection_handle asio_multiplexer::new_tcp_scribe(const std::string& host,
                                                   uint16_t port) {
  default_socket fd{new_tcp_connection(backend(), host, port)};
  auto id = int64_from_native_socket(fd.native_handle());
  std::lock_guard<std::mutex> lock(mtx_sockets_);
  unassigned_sockets_.insert(std::make_pair(id, std::move(fd)));
  return connection_handle::from_int(id);
}

void asio_multiplexer::assign_tcp_scribe(abstract_broker* self,
                                         connection_handle hdl) {
  std::lock_guard<std::mutex> lock(mtx_sockets_);
  auto itr = unassigned_sockets_.find(hdl.id());
  if (itr != unassigned_sockets_.end()) {
    add_tcp_scribe(self, std::move(itr->second));
    unassigned_sockets_.erase(itr);
  }
}

template <class Socket>
connection_handle asio_multiplexer::add_tcp_scribe(abstract_broker* self,
                                                   Socket&& sock) {
  BOOST_ACTOR_LOG_TRACE("");
  class impl : public scribe {
  public:
    impl(abstract_broker* ptr, Socket&& s)
        : scribe(ptr, network::conn_hdl_from_socket(s)),
          launched_(false),
          stream_(s.get_io_service()) {
      stream_.init(std::move(s));
    }
    void configure_read(receive_policy::config config) override {
      BOOST_ACTOR_LOG_TRACE("");
      stream_.configure_read(config);
      if (! launched_) {
        launch();
      }
    }
    std::vector<char>& wr_buf() override {
      return stream_.wr_buf();
    }
    std::vector<char>& rd_buf() override {
      return stream_.rd_buf();
    }
    void stop_reading() override {
      BOOST_ACTOR_LOG_TRACE("");
      stream_.stop_reading();
      detach(false);
    }
    void flush() override {
      BOOST_ACTOR_LOG_TRACE("");
      stream_.flush(this);
    }
    void launch() {
      BOOST_ACTOR_LOG_TRACE("");
      BOOST_ACTOR_ASSERT(! launched_);
      launched_ = true;
      stream_.start(this);
    }

 private:
    bool launched_;
    stream<Socket> stream_;
  };
  auto ptr = make_counted<impl>(self, std::move(sock));
  self->add_scribe(ptr);
  return ptr->hdl();
}

connection_handle asio_multiplexer::add_tcp_scribe(abstract_broker* self,
                                                   native_socket fd) {
  BOOST_ACTOR_LOG_TRACE(BOOST_ACTOR_ARG(self) << ", " << BOOST_ACTOR_ARG(fd));
  boost::system::error_code ec;
  default_socket sock{backend()};
  sock.assign(boost::asio::ip::tcp::v6(), fd, ec);
  if (ec) {
    sock.assign(boost::asio::ip::tcp::v4(), fd, ec);
  }
  if (ec) {
    throw network_error(ec.message());
  }
  return add_tcp_scribe(self, std::move(sock));
}

connection_handle asio_multiplexer::add_tcp_scribe(abstract_broker* self,
                                                   const std::string& host,
                                                   uint16_t port) {
  BOOST_ACTOR_LOG_TRACE(BOOST_ACTOR_ARG(self) << ", " << BOOST_ACTOR_ARG(host) << ":" << BOOST_ACTOR_ARG(port));
  return add_tcp_scribe(self, new_tcp_connection(backend(), host, port));
}

std::pair<accept_handle, uint16_t>
asio_multiplexer::new_tcp_doorman(uint16_t port, const char* in, bool rflag) {
  BOOST_ACTOR_LOG_TRACE(BOOST_ACTOR_ARG(port) << ", addr = " << (in ? in : "nullptr"));
  default_socket_acceptor fd{backend()};
  ip_bind(fd, port, in, rflag);
  auto id = int64_from_native_socket(fd.native_handle());
  auto assigned_port = fd.local_endpoint().port();
  std::lock_guard<std::mutex> lock(mtx_acceptors_);
  unassigned_acceptors_.insert(std::make_pair(id, std::move(fd)));
  return {accept_handle::from_int(id), assigned_port};
}

void asio_multiplexer::assign_tcp_doorman(abstract_broker* self, accept_handle hdl) {
  BOOST_ACTOR_LOG_TRACE("");
  std::lock_guard<std::mutex> lock(mtx_acceptors_);
  auto itr = unassigned_acceptors_.find(hdl.id());
  if (itr != unassigned_acceptors_.end()) {
    add_tcp_doorman(self, std::move(itr->second));
    unassigned_acceptors_.erase(itr);
  }
}

accept_handle
asio_multiplexer::add_tcp_doorman(abstract_broker* self,
                                  default_socket_acceptor&& sock) {
  BOOST_ACTOR_LOG_TRACE("sock.fd = " << sock.native_handle());
  BOOST_ACTOR_ASSERT(sock.native_handle() != network::invalid_native_socket);
  class impl : public doorman {
  public:
    impl(abstract_broker* ptr, default_socket_acceptor&& s,
         network::asio_multiplexer& am)
        : doorman(ptr, network::accept_hdl_from_socket(s),
                  s.local_endpoint().port()),
          acceptor_(am, s.get_io_service()) {
      acceptor_.init(std::move(s));
    }
    void new_connection() override {
      BOOST_ACTOR_LOG_TRACE("");
      auto& am = acceptor_.backend();
      accept_msg().handle
        = am.add_tcp_scribe(parent(), std::move(acceptor_.accepted_socket()));
      parent()->invoke_message(invalid_actor_addr, invalid_message_id,
                               accept_msg_);
    }
    void stop_reading() override {
      BOOST_ACTOR_LOG_TRACE("");
      acceptor_.stop();
      detach(false);
    }
    void launch() override {
      BOOST_ACTOR_LOG_TRACE("");
      acceptor_.start(this);
    }

  private:
    network::acceptor<default_socket_acceptor> acceptor_;
  };
  auto ptr = make_counted<impl>(self, std::move(sock), *this);
  self->add_doorman(ptr);
  return ptr->hdl();
}

accept_handle asio_multiplexer::add_tcp_doorman(abstract_broker* self,
                                                native_socket fd) {
  default_socket_acceptor sock{backend()};
  boost::system::error_code ec;
  sock.assign(boost::asio::ip::tcp::v6(), fd, ec);
  if (ec) {
    sock.assign(boost::asio::ip::tcp::v4(), fd, ec);
  }
  if (ec) {
    throw network_error(ec.message());
  }
  return add_tcp_doorman(self, std::move(sock));
}

std::pair<accept_handle, uint16_t>
asio_multiplexer::add_tcp_doorman(abstract_broker* self, uint16_t port, const char* in,
                                  bool rflag) {
  default_socket_acceptor fd{backend()};
  ip_bind(fd, port, in, rflag);
  auto p = fd.local_endpoint().port();
  return {add_tcp_doorman(self, std::move(fd)), p};
}

void asio_multiplexer::dispatch_runnable(runnable_ptr ptr) {
  backend().post([=]() {
    ptr->run();
  });
}

asio_multiplexer::asio_multiplexer() {
  // nop
}

asio_multiplexer::~asio_multiplexer() {
  //nop
}

multiplexer::supervisor_ptr asio_multiplexer::make_supervisor() {
  return std::unique_ptr<asio_supervisor>(new asio_supervisor(backend()));
}

void asio_multiplexer::run() {
  BOOST_ACTOR_LOG_TRACE("asio-based multiplexer");
  boost::system::error_code ec;
  backend().run(ec);
  if (ec) {
    throw std::runtime_error(ec.message());
  }
}

boost::asio::io_service* asio_multiplexer::pimpl() {
  return &backend_;
}

asio_multiplexer& get_multiplexer_singleton() {
  return static_cast<asio_multiplexer&>(middleman::instance()->backend());
}

} // namesapce network
} // namespace io
} // namespace actor
} // namespace boost
