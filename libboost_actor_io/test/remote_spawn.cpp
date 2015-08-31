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

#define BOOST_TEST_MODULE io_remote_spawn
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

#include "boost/actor/experimental/announce_actor_type.hpp"

#ifdef BOOST_ACTOR_USE_ASIO
#include "boost/actor/io/network/asio_multiplexer.hpp"
#endif // BOOST_ACTOR_USE_ASIO

using boost::none;
using boost::join;
using boost::variant;
using boost::optional;
using boost::is_any_of;
using boost::token_compress_on;
using namespace boost::actor;
using namespace boost::actor::experimental;

namespace {

behavior mirror(event_based_actor* self) {
  return {
    others >> [=] {
      return self->current_message();
    }
  };
}

behavior client(event_based_actor* self, actor serv) {
  self->send(serv, ok_atom::value);
  return {
    others >> [=] {
      BOOST_ERROR("unexpected message: "
                     << to_string(self->current_message()));
    }
  };
}

struct server_state {
  actor client; // the spawn we connect to the server in our main
  actor aut; // our mirror
};

behavior server(stateful_actor<server_state>* self) {
  self->on_sync_failure([=] {
    BOOST_ERROR("unexpected sync response: "
                   << to_string(self->current_message()));
  });
  return {
    [=](ok_atom) {
      auto s = self->current_sender();
      BOOST_REQUIRE(s != invalid_actor_addr);
      BOOST_REQUIRE(s.is_remote());
      self->state.client = actor_cast<actor>(s);
      auto mm = io::get_middleman_actor();
      self->sync_send(mm, spawn_atom::value,
                      s.node(), "mirror", make_message()).then(
        [=](ok_atom, const actor_addr& addr, const std::set<std::string>& ifs) {
          BOOST_REQUIRE(addr != invalid_actor_addr);
          BOOST_CHECK(ifs.empty());
          self->state.aut = actor_cast<actor>(addr);
          self->send(self->state.aut, "hello mirror");
          self->become(
            [=](const std::string& str) {
              BOOST_CHECK(self->current_sender() == self->state.aut);
              BOOST_CHECK(str == "hello mirror");
              self->send_exit(self->state.aut, exit_reason::kill);
              self->send_exit(self->state.client, exit_reason::kill);
              self->quit();
            }
          );
        },
        [=](error_atom, const std::string& errmsg) {
          BOOST_ERROR("could not spawn mirror: " << errmsg);
        }
      );
    }
  };
}

} // namespace <anonymous>

BOOST_AUTO_TEST_CASE(remote_spawn) {
  announce_actor_type("mirror", mirror);
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
    {"server,s", "run as server (don't run client"},
    {"client,c", "add client port (two needed)", port},
    {"port,p", "force a port in server mode", port},
    {"use-asio", "use ASIO network backend (if available)"}
  });
  if (! r.error.empty() || r.opts.count("help") > 0 || ! r.remainder.empty()) {
    std::cout << r.error << std::endl << std::endl << r.helptext << std::endl;
    return;
  }
  auto use_asio = r.opts.count("use-asio") > 0;
# ifdef BOOST_ACTOR_USE_ASIO
  if (use_asio) {
    BOOST_TEST_MESSAGE("enable ASIO backend");
    io::set_middleman<io::network::asio_multiplexer>();
  }
# endif // BOOST_ACTOR_USE_ASIO
  if (r.opts.count("client") > 0) {
    auto serv = io::remote_actor("localhost", port);
    spawn(client, serv);
    await_all_actors_done();
    return;
  }
  auto serv = spawn(server);
  port = io::publish(serv, port);
  BOOST_TEST_MESSAGE("published server at port " << port);
  if (r.opts.count("server") == 0) {
    BOOST_TEST_MESSAGE("run client program");
    auto child = detail::run_sub_unit_test(invalid_actor,
                                           boost::unit_test::framework::master_test_suite().argv[0],
                                           0 /* max runtime dummy */,
                                           BOOST_PP_STRINGIZE(BOOST_TEST_MODULE),
                                           use_asio,
                                           {"--client="
                                            + std::to_string(port)});
    child.join();
  }
  await_all_actors_done();
  shutdown();
}
