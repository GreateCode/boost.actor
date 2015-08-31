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

#define BOOST_TEST_MODULE local_group
#include <boost/test/included/unit_test.hpp>

#include <chrono>

#include "boost/actor/all.hpp"

using boost::none;
using boost::join;
using boost::variant;
using boost::optional;
using boost::is_any_of;
using boost::token_compress_on;
using namespace boost::actor;

using msg_atom = atom_constant<atom("msg")>;
using timeout_atom = atom_constant<atom("timeout")>;

void testee(event_based_actor* self) {
  auto counter = std::make_shared<int>(0);
  auto grp = group::get("local", "test");
  self->join(grp);
  BOOST_TEST_MESSAGE("self joined group");
  self->become(
    [=](msg_atom) {
      BOOST_TEST_MESSAGE("received `msg_atom`");
      ++*counter;
      self->leave(grp);
      self->send(grp, msg_atom::value);
    },
    [=](timeout_atom) {
      // this actor should receive only 1 message
      BOOST_CHECK(*counter == 1);
      self->quit();
    }
  );
  self->send(grp, msg_atom::value);
  self->delayed_send(self, std::chrono::seconds(1), timeout_atom::value);
}

BOOST_AUTO_TEST_CASE(test_local_group) {
  spawn(testee);
  await_all_actors_done();
  shutdown();
}
