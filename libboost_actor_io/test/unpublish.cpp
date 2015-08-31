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

#define BOOST_TEST_MODULE io_unpublish
#include <boost/test/included/unit_test.hpp>

#include <thread>
#include <atomic>

#include "boost/actor/all.hpp"
#include "boost/actor/io/all.hpp"

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

namespace {

std::atomic<long> s_dtor_called;

class dummy : public event_based_actor {
public:
  ~dummy() {
    ++s_dtor_called;
  }
  behavior make_behavior() override {
    return {
      others >> [&] {
        BOOST_ERROR("Unexpected message: " << to_string(current_message()));
      }
    };
  }
};

void test_invalid_unpublish(const actor& published, uint16_t port) {
  auto d = spawn<dummy>();
  io::unpublish(d, port);
  auto ra = io::remote_actor("127.0.0.1", port);
  BOOST_CHECK(ra != d);
  BOOST_CHECK(ra == published);
  anon_send_exit(d, exit_reason::user_shutdown);
}

BOOST_AUTO_TEST_CASE(unpublishing) {
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
  if (argc == 1 && strcmp(argv[0], "--use-asio") == 0) {
#   ifdef BOOST_ACTOR_USE_ASIO
    BOOST_TEST_MESSAGE("enable ASIO backend");
    io::set_middleman<io::network::asio_multiplexer>();
#   endif // BOOST_ACTOR_USE_ASIO
  }
  { // scope for local variables
    auto d = spawn<dummy>();
    auto port = io::publish(d, 0);
    BOOST_TEST_MESSAGE("published actor on port " << port);
    test_invalid_unpublish(d, port);
    BOOST_TEST_MESSAGE("finished `invalid_unpublish`");
    io::unpublish(d, port);
    // must fail now
    try {
      BOOST_TEST_MESSAGE("expect exception...");
      io::remote_actor("127.0.0.1", port);
      BOOST_ERROR("unexpected: remote actor succeeded!");
    } catch (network_error&) {
      BOOST_TEST_MESSAGE("unpublish succeeded");
    }
    anon_send_exit(d, exit_reason::user_shutdown);
  }
  await_all_actors_done();
  shutdown();
  BOOST_CHECK_EQUAL(s_dtor_called.load(), 2);
}

} // namespace <anonymous>
