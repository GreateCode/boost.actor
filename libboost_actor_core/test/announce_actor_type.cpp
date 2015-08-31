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

#define BOOST_TEST_MODULE announce_actor_type
#include <boost/test/included/unit_test.hpp>

#include "boost/actor/all.hpp"

#include "boost/actor/experimental/announce_actor_type.hpp"

#include "boost/actor/detail/actor_registry.hpp"

using boost::none;
using boost::join;
using boost::variant;
using boost::optional;
using boost::is_any_of;
using boost::token_compress_on;
using namespace boost::actor;
using namespace boost::actor::experimental;

using std::endl;

namespace {

struct fixture {
  actor aut;
  actor spawner;

  fixture() {
    auto registry = detail::singletons::get_actor_registry();
    spawner = registry->get_named(atom("SpawnServ"));
  }

  void set_aut(message args, bool expect_fail = false) {
    BOOST_TEST_MESSAGE("set aut");
    scoped_actor self;
    self->on_sync_failure([&] {
      BOOST_ERROR("received unexpeced sync. response: "
                     << to_string(self->current_message()));
    });
    if (expect_fail) {
      self->sync_send(spawner, get_atom::value, "test_actor", std::move(args)).await(
        [&](error_atom, const std::string&) {
          BOOST_TEST_MESSAGE("received error_atom (expected)");
        }
      );
    } else {
      self->sync_send(spawner, get_atom::value, "test_actor", std::move(args)).await(
        [&](ok_atom, actor_addr res, const std::set<std::string>& ifs) {
          BOOST_REQUIRE(res != invalid_actor_addr);
          aut = actor_cast<actor>(res);
          BOOST_CHECK(ifs.empty());
        }
      );
    }
  }

  ~fixture() {
    if (aut != invalid_actor) {
      scoped_actor self;
      self->monitor(aut);
      self->receive(
        [](const down_msg& dm) {
          BOOST_CHECK(dm.reason == exit_reason::normal);
        }
      );
    }
    await_all_actors_done();
    shutdown();
  }
};

} // namespace <anonymous>

BOOST_FIXTURE_TEST_SUITE(announce_actor_type_tests, fixture)

BOOST_AUTO_TEST_CASE(fun_no_args) {
  auto test_actor = [] {
    BOOST_TEST_MESSAGE("inside test_actor");
  };
  announce_actor_type("test_actor", test_actor);
  set_aut(make_message());
}

BOOST_AUTO_TEST_CASE(fun_no_args_selfptr) {
  auto test_actor = [](event_based_actor*) {
    BOOST_TEST_MESSAGE("inside test_actor");
  };
  announce_actor_type("test_actor", test_actor);
  set_aut(make_message());
}
BOOST_AUTO_TEST_CASE(fun_one_arg) {
  auto test_actor = [](int i) {
    BOOST_CHECK_EQUAL(i, 42);
  };
  announce_actor_type("test_actor", test_actor);
  set_aut(make_message(42));
}

BOOST_AUTO_TEST_CASE(fun_one_arg_selfptr) {
  auto test_actor = [](event_based_actor*, int i) {
    BOOST_CHECK_EQUAL(i, 42);
  };
  announce_actor_type("test_actor", test_actor);
  set_aut(make_message(42));
}

BOOST_AUTO_TEST_CASE(class_no_arg) {
  struct test_actor : event_based_actor {
    behavior make_behavior() override {
      return {};
    }
  };
  announce_actor_type<test_actor>("test_actor");
  set_aut(make_message(42), true);
  set_aut(make_message());
}

BOOST_AUTO_TEST_CASE(class_one_arg) {
  struct test_actor : event_based_actor {
    test_actor(int value) {
      BOOST_CHECK_EQUAL(value, 42);
    }
    behavior make_behavior() override {
      return {};
    }
  };
  announce_actor_type<test_actor, const int&>("test_actor");
  set_aut(make_message(), true);
  set_aut(make_message(42));
}

BOOST_AUTO_TEST_SUITE_END()
