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

#define BOOST_TEST_MODULE constructor_attach
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

using die_atom = atom_constant<atom("die")>;
using done_atom = atom_constant<atom("done")>;

} // namespace <anonymous>

BOOST_AUTO_TEST_CASE(constructor_attach) {
  class testee : public event_based_actor {
  public:
    explicit testee(actor buddy) {
      attach_functor([=](uint32_t reason) {
        send(buddy, done_atom::value, reason);
      });
    }

    behavior make_behavior() {
      return {
        [=](die_atom) {
          quit(exit_reason::user_shutdown);
        }
      };
    }
  };
  class spawner : public event_based_actor {
  public:
    spawner() : downs_(0) {
      // nop
    }

    behavior make_behavior() {
      testee_ = spawn<testee, monitored>(this);
      return {
        [=](const down_msg& msg) {
          BOOST_CHECK_EQUAL(msg.reason, exit_reason::user_shutdown);
          BOOST_CHECK(msg.source == testee_);
          if (++downs_ == 2) {
            quit(msg.reason);
          }
        },
        [=](done_atom, uint32_t reason) {
          BOOST_CHECK_EQUAL(reason, exit_reason::user_shutdown);
          if (++downs_ == 2) {
            quit(reason);
          }
        },
        others >> [=] {
          forward_to(testee_);
        }
      };
    }

    void on_exit() {
      testee_ = invalid_actor;
    }

  private:
    int downs_;
    actor testee_;
  };
  anon_send(spawn<spawner>(), die_atom::value);
  await_all_actors_done();
  shutdown();
}
