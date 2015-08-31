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

#include "boost/actor/config.hpp"

#define BOOST_TEST_MODULE io_dynamic_broker
#include <boost/test/included/unit_test.hpp>

#include <memory>
#include <iostream>

#include "boost/actor/all.hpp"
#include "boost/actor/io/all.hpp"

#include <boost/algorithm/string.hpp>

#include "boost/actor/detail/run_sub_unit_test.hpp"

#ifdef BOOST_ACTOR_USE_ASIO
#include "boost/actor/io/network/asio_multiplexer.hpp"
#endif // BOOST_ACTOR_USE_ASIO

using namespace std;
using boost::none;
using boost::join;
using boost::variant;
using boost::optional;
using boost::is_any_of;
using boost::token_compress_on;
using namespace boost::actor;
using namespace boost::actor::io;

namespace {

using ping_atom = boost::actor::atom_constant<boost::actor::atom("ping")>;
using pong_atom = boost::actor::atom_constant<boost::actor::atom("pong")>;
using publish_atom = boost::actor::atom_constant<boost::actor::atom("publish")>;
using kickoff_atom = boost::actor::atom_constant<boost::actor::atom("kickoff")>;

void ping(event_based_actor* self, size_t num_pings) {
  BOOST_TEST_MESSAGE("num_pings: " << num_pings);
  auto count = std::make_shared<size_t>(0);
  self->become(
    [=](kickoff_atom, const actor& pong) {
      BOOST_TEST_MESSAGE("received `kickoff_atom`");
      self->send(pong, ping_atom::value, 1);
      self->become(
      [=](pong_atom, int value)->std::tuple<atom_value, int> {
        if (++*count >= num_pings) {
          BOOST_TEST_MESSAGE("received " << num_pings
                      << " pings, call self->quit");
          self->quit();
        }
        return std::make_tuple(ping_atom::value, value + 1);
      },
      others >> [=] {
        BOOST_ERROR("Unexpected message: "
                       << to_string(self->current_message()));
      });
    },
    others >> [=] {
      BOOST_ERROR("Unexpected message: "
                     << to_string(self->current_message()));
    }
  );
}

void pong(event_based_actor* self) {
  BOOST_TEST_MESSAGE("pong actor started");
  self->become(
    [=](ping_atom, int value) -> std::tuple<atom_value, int> {
      BOOST_TEST_MESSAGE("received `ping_atom`");
      self->monitor(self->current_sender());
      // set next behavior
      self->become(
        [](ping_atom, int val) {
          return std::make_tuple(pong_atom::value, val);
        },
        [=](const down_msg& dm) {
          BOOST_TEST_MESSAGE("received down_msg{" << dm.reason << "}");
          self->quit(dm.reason);
        },
        others >> [=] {
          BOOST_ERROR("Unexpected message: "
                         << to_string(self->current_message()));
        }
      );
      // reply to 'ping'
      return std::make_tuple(pong_atom::value, value);
    },
    others >> [=] {
      BOOST_ERROR("Unexpected message: "
                     << to_string(self->current_message()));
    }
  );
}

void peer_fun(broker* self, connection_handle hdl, const actor& buddy) {
  BOOST_TEST_MESSAGE("peer_fun called");
  BOOST_CHECK(self != nullptr);
  BOOST_CHECK(buddy != invalid_actor);
  self->monitor(buddy);
  // assume exactly one connection
  auto cons = self->connections();
  if (cons.size() != 1) {
    cerr << "expected 1 connection, found " << cons.size() << endl;
    throw std::logic_error("num_connections() != 1");
  }
  self->configure_read(
    hdl, receive_policy::exactly(sizeof(atom_value) + sizeof(int)));
  auto write = [=](atom_value type, int value) {
    auto& buf = self->wr_buf(hdl);
    auto first = reinterpret_cast<char*>(&type);
    buf.insert(buf.end(), first, first + sizeof(atom_value));
    first = reinterpret_cast<char*>(&value);
    buf.insert(buf.end(), first, first + sizeof(int));
    self->flush(hdl);

  };
  self->become(
    [=](const connection_closed_msg&) {
      BOOST_TEST_MESSAGE("received connection_closed_msg");
      self->quit();
    },
    [=](const new_data_msg& msg) {
      BOOST_TEST_MESSAGE("received new_data_msg");
      atom_value type;
      int value;
      memcpy(&type, msg.buf.data(), sizeof(atom_value));
      memcpy(&value, msg.buf.data() + sizeof(atom_value), sizeof(int));
      self->send(buddy, type, value);
    },
    [=](ping_atom, int value) {
      BOOST_TEST_MESSAGE("received ping{" << value << "}");
      write(ping_atom::value, value);
    },
    [=](pong_atom, int value) {
      BOOST_TEST_MESSAGE("received pong{" << value << "}");
      write(pong_atom::value, value);
    },
    [=](const down_msg& dm) {
      BOOST_TEST_MESSAGE("received down_msg");
      if (dm.source == buddy) {
        self->quit(dm.reason);
      }
    },
    others >> [=] {
      BOOST_ERROR("Unexpected message: "
                     << to_string(self->current_message()));
    }
  );
}

behavior peer_acceptor_fun(broker* self, const actor& buddy) {
  BOOST_TEST_MESSAGE("peer_acceptor_fun");
  return {
    [=](const new_connection_msg& msg) {
      BOOST_TEST_MESSAGE("received `new_connection_msg`");
      self->fork(peer_fun, msg.handle, buddy);
      self->quit();
    },
    [=](publish_atom) {
      return self->add_tcp_doorman(0, "127.0.0.1").second;
    },
    others >> [&] {
      BOOST_ERROR("Unexpected message: "
                     << to_string(self->current_message()));
    }
  };
}

void run_server(bool spawn_client, const char* bin_path, bool use_asio) {
  scoped_actor self;
  auto serv = io::spawn_io(peer_acceptor_fun, spawn(pong));
  self->sync_send(serv, publish_atom::value).await(
    [&](uint16_t port) {
      BOOST_TEST_MESSAGE("server is running on port " << port);
      if (spawn_client) {
        auto child = detail::run_sub_unit_test(self,
                                               bin_path,
                                               0 /* max runtime dummy */,
                                               BOOST_PP_STRINGIZE(BOOST_TEST_MODULE),
                                               use_asio,
                                               {"--client-port="
                                                + std::to_string(port)});
        BOOST_TEST_MESSAGE("block till child process has finished");
        child.join();
      }
    }
  );
  self->await_all_other_actors_done();
  self->receive(
    [](const std::string& output) {
      cout << endl << endl << "*** output of client program ***"
           << endl << output << endl;
    }
  );
}

} // namespace <anonymous>

BOOST_AUTO_TEST_CASE(test_broker) {
  auto argv = boost::unit_test::framework::master_test_suite().argv;
  auto argc = boost::unit_test::framework::master_test_suite().argc;
  // skip everything in argv until the separator "--" is found
  auto cmp = [](const char* x) { return strcmp(x, "--") == 0; };
  auto argv_end = argv + argc;
  auto argv_sep = std::find_if(argv, argv_end, cmp);
  if (argv_sep == argv_end) {
    argc = 0;
  } else {
    argv = argv_sep + 1;
    argc = std::distance(argv, argv_end);
  }
  uint16_t port = 0;
  auto r = message_builder(argv, argv + argc).extract_opts({
    {"client-port,c", "set port for IO client", port},
    {"server,s", "run in server mode"},
    {"use-asio", "use ASIO network backend (if available)"}
  });
  if (! r.error.empty() || r.opts.count("help") > 0 || ! r.remainder.empty()) {
    cout << r.error << endl << endl << r.helptext << endl;
    return;
  }
  auto use_asio = r.opts.count("use-asio") > 0;
  if (use_asio) {
#   ifdef BOOST_ACTOR_USE_ASIO
    BOOST_TEST_MESSAGE("enable ASIO backend");
    io::set_middleman<io::network::asio_multiplexer>();
#   endif // BOOST_ACTOR_USE_ASIO
  }
  if (r.opts.count("client-port") > 0) {
    auto p = spawn(ping, 10);
    BOOST_TEST_MESSAGE("spawn_io_client...");
    auto cl = spawn_io_client(peer_fun, "localhost", port, p);
    BOOST_TEST_MESSAGE("spawn_io_client finished");
    anon_send(p, kickoff_atom::value, cl);
    BOOST_TEST_MESSAGE("`kickoff_atom` has been send");
  } else if (r.opts.count("server") > 0) {
    // run in server mode
    run_server(false, argv[0], use_asio);
  } else {
    run_server(true, boost::unit_test::framework::master_test_suite().argv[0], use_asio);
  }
  BOOST_TEST_MESSAGE("block on `await_all_actors_done`");
  await_all_actors_done();
  BOOST_TEST_MESSAGE("`await_all_actors_done` has finished");
  shutdown();
}
