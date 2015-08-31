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

#define BOOST_TEST_MODULE serial_reply
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

using hi_atom = atom_constant<atom("hi")>;
using ho_atom = atom_constant<atom("ho")>;
using sub0_atom = atom_constant<atom("sub0")>;
using sub1_atom = atom_constant<atom("sub1")>;
using sub2_atom = atom_constant<atom("sub2")>;
using sub3_atom = atom_constant<atom("sub3")>;
using sub4_atom = atom_constant<atom("sub4")>;

} // namespace <anonymous>


BOOST_AUTO_TEST_CASE(test_serial_reply) {
  auto mirror_behavior = [=](event_based_actor* self) {
    self->become(others >> [=]() -> message {
      BOOST_TEST_MESSAGE("return self->current_message()");
      return self->current_message();
    });
  };
  auto master = spawn([=](event_based_actor* self) {
    BOOST_TEST_MESSAGE("ID of master: " << self->id());
    // spawn 5 mirror actors
    auto c0 = self->spawn<linked>(mirror_behavior);
    auto c1 = self->spawn<linked>(mirror_behavior);
    auto c2 = self->spawn<linked>(mirror_behavior);
    auto c3 = self->spawn<linked>(mirror_behavior);
    auto c4 = self->spawn<linked>(mirror_behavior);
    self->become (
      [=](hi_atom) -> continue_helper {
        BOOST_TEST_MESSAGE("received 'hi there'");
        return self->sync_send(c0, sub0_atom::value).then(
          [=](sub0_atom) -> continue_helper {
            BOOST_TEST_MESSAGE("received 'sub0'");
            return self->sync_send(c1, sub1_atom::value).then(
              [=](sub1_atom) -> continue_helper {
                BOOST_TEST_MESSAGE("received 'sub1'");
                return self->sync_send(c2, sub2_atom::value).then(
                  [=](sub2_atom) -> continue_helper {
                    BOOST_TEST_MESSAGE("received 'sub2'");
                    return self->sync_send(c3, sub3_atom::value).then(
                      [=](sub3_atom) -> continue_helper {
                        BOOST_TEST_MESSAGE("received 'sub3'");
                        return self->sync_send(c4, sub4_atom::value).then(
                          [=](sub4_atom) -> atom_value {
                            BOOST_TEST_MESSAGE("received 'sub4'");
                            return ho_atom::value;
                          }
                        );
                      }
                    );
                  }
                );
              }
            );
          }
        );
      }
    );
  });
  { // lifetime scope of self
    scoped_actor self;
    BOOST_TEST_MESSAGE("ID of main: " << self->id());
    self->sync_send(master, hi_atom::value).await(
      [](ho_atom) {
        BOOST_TEST_MESSAGE("received 'ho'");
      },
      others >> [&] {
        BOOST_ERROR("Unexpected message: "
                       << to_string(self->current_message()));
      }
    );
    self->send_exit(master, exit_reason::user_shutdown);
  }
  await_all_actors_done();
  shutdown();
}
