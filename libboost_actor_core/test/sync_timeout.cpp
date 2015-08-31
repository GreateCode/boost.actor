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

#define BOOST_TEST_MODULE sync_timeout
#include <boost/test/included/unit_test.hpp>

#include <thread>
#include <chrono>

#include "boost/actor/all.hpp"

using boost::none;
using boost::join;
using boost::variant;
using boost::optional;
using boost::is_any_of;
using boost::token_compress_on;
using namespace boost::actor;

namespace {

using ping_atom = atom_constant<atom("ping")>;
using pong_atom = atom_constant<atom("pong")>;
using send_ping_atom = atom_constant<atom("send_ping")>;

behavior pong() {
  return {
    [=] (ping_atom) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
      return pong_atom::value;
    }
  };
}

behavior ping1(event_based_actor* self, const actor& pong_actor) {
  self->link_to(pong_actor);
  self->send(self, send_ping_atom::value);
  return {
    [=](send_ping_atom) {
      self->sync_send(pong_actor, ping_atom::value).then(
        [=](pong_atom) {
          BOOST_ERROR("received pong atom");
          self->quit(exit_reason::user_shutdown);
        },
        after(std::chrono::milliseconds(100)) >> [=] {
          BOOST_TEST_MESSAGE("sync timeout: check");
          self->quit(exit_reason::user_shutdown);
        }
      );
    }
  };
}

behavior ping2(event_based_actor* self, const actor& pong_actor) {
  self->link_to(pong_actor);
  self->send(self, send_ping_atom::value);
  auto received_inner = std::make_shared<bool>(false);
  return {
    [=](send_ping_atom) {
      self->sync_send(pong_actor, ping_atom::value).then(
        [=](pong_atom) {
          BOOST_ERROR("received pong atom");
          self->quit(exit_reason::user_shutdown);
        },
        after(std::chrono::milliseconds(100)) >> [=] {
          BOOST_TEST_MESSAGE("inner timeout: check");
          *received_inner = true;
        }
      );
    },
    after(std::chrono::milliseconds(100)) >> [=] {
      BOOST_CHECK_EQUAL(*received_inner, true);
      self->quit(exit_reason::user_shutdown);
    }
  };
}

struct fixture {
  ~fixture() {
    await_all_actors_done();
    shutdown();
  }
};

} // namespace <anonymous>

BOOST_FIXTURE_TEST_SUITE(atom_tests, fixture)

BOOST_AUTO_TEST_CASE(single_timeout) {
  spawn(ping1, spawn(pong));
}

BOOST_AUTO_TEST_CASE(scoped_timeout) {
  spawn(ping2, spawn(pong));
}

BOOST_AUTO_TEST_SUITE_END()
