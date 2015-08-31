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

#define BOOST_TEST_MODULE either
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

using foo = typed_actor<replies_to<int>::with_either<int>::or_else<float>>;

foo::behavior_type my_foo() {
  return {
    [](int arg) -> either<int>::or_else<float> {
      if (arg == 42) {
        return 42;
      }
      return static_cast<float>(arg);
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

BOOST_AUTO_TEST_CASE(basic_usage) {
  auto f1 = []() -> either<int>::or_else<float> {
    return 42;
  };
  auto f2 = []() -> either<int>::or_else<float> {
    return 42.f;
  };
  auto f3 = [](bool flag) -> either<int, int>::or_else<float, float> {
    if (flag) {
      return {1, 2};
    }
    return {3.f, 4.f};
  };
  BOOST_CHECK(f1().value == make_message(42));
  BOOST_CHECK(f2().value == make_message(42.f));
  BOOST_CHECK(f3(true).value == make_message(1, 2));
  BOOST_CHECK(f3(false).value == make_message(3.f, 4.f));
  either<int>::or_else<float> x1{4};
  BOOST_CHECK(x1.value == make_message(4));
  either<int>::or_else<float> x2{4.f};
  BOOST_CHECK(x2.value == make_message(4.f));
}

BOOST_AUTO_TEST_CASE(either_in_typed_interfaces) {
  auto mf = spawn(my_foo);
  scoped_actor self;
  self->sync_send(mf, 42).await(
    [](int val) {
      BOOST_CHECK_EQUAL(val, 42);
    },
    [](float) {
      BOOST_ERROR("expected an integer");
    }
  );
  self->sync_send(mf, 10).await(
    [](int) {
      BOOST_ERROR("expected a float");
    },
    [](float val) {
      BOOST_CHECK_EQUAL(val, 10.f);
    }
  );
  self->send_exit(mf, exit_reason::kill);
}

BOOST_AUTO_TEST_SUITE_END()
