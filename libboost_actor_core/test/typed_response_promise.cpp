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

#include <map>

#include "boost/actor/config.hpp"

#define BOOST_TEST_MODULE typed_response_promise
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

using foo_actor = typed_actor<replies_to<get_atom, int>::with<int>,
                              reacts_to<put_atom, int, int>>;

using foo_promise = typed_response_promise<int>;

class foo_actor_impl : public foo_actor::base {
public:
  behavior_type make_behavior() override {
    return {
      [=](get_atom, int x) -> foo_promise {
        auto calculator = spawn([](event_based_actor* self) -> behavior {
          return {
            [self](int promise_id, int value) -> message {
              self->quit();
              return make_message(put_atom::value, promise_id, value * 2);
            }
          };
        });
        send(calculator, next_id_, x);
        auto& entry = promises_[next_id_++];
        entry = make_response_promise();
        return entry;
      },
      [=](put_atom, int promise_id, int x) {
        auto i = promises_.find(promise_id);
        if (i == promises_.end())
          return;
        i->second.deliver(x);
        promises_.erase(i);
      }
    };
  }

private:
  int next_id_ = 0;
  std::map<int, foo_promise> promises_;
};

struct fixture {
  ~fixture() {
    await_all_actors_done();
    shutdown();
  }
};

} // namespace <anonymous>

BOOST_FIXTURE_TEST_SUITE(typed_spawn_tests, fixture)

BOOST_AUTO_TEST_CASE(typed_response_promise) {
  scoped_actor self;
  auto foo = spawn<foo_actor_impl>();
  self->sync_send(foo, get_atom::value, 42).await(
    [](int x) {
      BOOST_CHECK_EQUAL(x, 84);
    }
  );
  self->send_exit(foo, exit_reason::user_shutdown);
}

BOOST_AUTO_TEST_SUITE_END()
