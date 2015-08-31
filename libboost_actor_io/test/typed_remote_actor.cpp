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

#define BOOST_TEST_MODULE io_typed_remote_actor
#include <boost/test/included/unit_test.hpp>

#include <thread>
#include <string>
#include <cstring>
#include <sstream>
#include <iostream>
#include <functional>

#include "boost/actor/all.hpp"
#include "boost/actor/io/all.hpp"

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

struct ping {
  int32_t value;
};

bool operator==(const ping& lhs, const ping& rhs) {
  return lhs.value == rhs.value;
}

struct pong {
  int32_t value;

};

bool operator==(const pong& lhs, const pong& rhs) {
  return lhs.value == rhs.value;
}

using server_type = typed_actor<replies_to<ping>::with<pong>>;

using client_type = typed_actor<>;

server_type::behavior_type server() {
  return {
    [](const ping & p) -> pong {
      BOOST_CHECK_EQUAL(p.value, 42);
      return pong{p.value};
    }
  };
}

void run_client(const char* host, uint16_t port) {
  // check whether invalid_argument is thrown
  // when trying to connect to get an untyped
  // handle to the server
  try {
    io::remote_actor(host, port);
  }
  catch (network_error& e) {
    BOOST_TEST_MESSAGE(e.what());
  }
  BOOST_TEST_MESSAGE("connect to typed_remote_actor");
  auto serv = io::typed_remote_actor<server_type>(host, port);
  scoped_actor self;
  self->sync_send(serv, ping{42})
    .await([](const pong& p) { BOOST_CHECK_EQUAL(p.value, 42); });
  anon_send_exit(serv, exit_reason::user_shutdown);
  self->monitor(serv);
  self->receive([&](const down_msg& dm) {
    BOOST_CHECK_EQUAL(dm.reason, exit_reason::user_shutdown);
    BOOST_CHECK(dm.source == serv);
  });
}

uint16_t run_server() {
  auto port = io::typed_publish(spawn(server), 0, "127.0.0.1");
  BOOST_TEST_MESSAGE("running on port " << port);
  return port;
}

BOOST_AUTO_TEST_CASE(test_typed_remote_actor) {
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
  announce<ping>("ping", &ping::value);
  announce<pong>("pong", &pong::value);
  uint16_t port = 0;
  auto r = message_builder(argv, argv + argc).extract_opts({
    {"client-port,c", "set port for client", port},
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
    BOOST_TEST_MESSAGE("run in client mode");
    run_client("localhost", port);
  } else if (r.opts.count("server") > 0) {
    BOOST_TEST_MESSAGE("run in server mode");
    run_server();
  } else {
    port = run_server();
    // execute client_part() in a separate process,
    // connected via localhost socket
    scoped_actor self;
    auto child = detail::run_sub_unit_test(self,
                                           boost::unit_test::framework::master_test_suite().argv[0],
                                           0 /* max runtime dummy */,
                                           BOOST_PP_STRINGIZE(BOOST_TEST_MODULE),
                                           use_asio,
                                           {"--client-port="
                                            + std::to_string(port)});
    BOOST_TEST_MESSAGE("block till child process has finished");
    child.join();
    self->await_all_other_actors_done();
    self->receive(
      [](const std::string& output) {
        cout << endl << endl << "*** output of client program ***"
             << endl << output << endl;
      }
    );
  }
  await_all_actors_done();
  shutdown();
}
