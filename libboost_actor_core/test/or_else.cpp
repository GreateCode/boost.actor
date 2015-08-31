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

#define BOOST_TEST_MODULE or_else
#include <boost/test/included/unit_test.hpp>

#include "boost/actor/all.hpp"

using boost::none;
using boost::join;
using boost::variant;
using boost::optional;
using boost::is_any_of;
using boost::token_compress_on;
using namespace boost::actor;

namespace {

using a_atom = atom_constant<atom("a")>;
using b_atom = atom_constant<atom("b")>;
using c_atom = atom_constant<atom("c")>;

message_handler handle_a() {
  return [](a_atom) { return 1; };
}

message_handler handle_b() {
  return [](b_atom) { return 2; };
}

message_handler handle_c() {
  return [](c_atom) { return 3; };
}

void run_testee(actor testee) {
  scoped_actor self;
  self->sync_send(testee, a_atom::value).await([](int i) {
    BOOST_CHECK_EQUAL(i, 1);
  });
  self->sync_send(testee, b_atom::value).await([](int i) {
    BOOST_CHECK_EQUAL(i, 2);
  });
  self->sync_send(testee, c_atom::value).await([](int i) {
    BOOST_CHECK_EQUAL(i, 3);
  });
  self->send_exit(testee, exit_reason::user_shutdown);
  self->await_all_other_actors_done();
}

struct fixture {
  ~fixture() {
    await_all_actors_done();
    shutdown();
  }
};

} // namespace <anonymous>

BOOST_FIXTURE_TEST_SUITE(atom_tests, fixture)

BOOST_AUTO_TEST_CASE(composition1) {
  run_testee(
    spawn([=] {
      return handle_a().or_else(handle_b()).or_else(handle_c());
    })
  );
}

BOOST_AUTO_TEST_CASE(composition2) {
  run_testee(
    spawn([=] {
      return handle_a().or_else(handle_b()).or_else([](c_atom) { return 3; });
    })
  );
}

BOOST_AUTO_TEST_CASE(composition3) {
  run_testee(
    spawn([=] {
      return message_handler{[](a_atom) { return 1; }}.
             or_else(handle_b()).or_else(handle_c());
    })
  );
}

BOOST_AUTO_TEST_SUITE_END()
