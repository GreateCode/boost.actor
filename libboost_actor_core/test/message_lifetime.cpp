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

#define BOOST_TEST_MODULE message_lifetime
#include <boost/test/included/unit_test.hpp>

#include <atomic>
#include <iostream>

#include "boost/actor/all.hpp"

using boost::none;
using boost::join;
using boost::variant;
using boost::optional;
using boost::is_any_of;
using boost::token_compress_on;
using namespace boost::actor;

namespace {

class testee : public event_based_actor {
public:
  testee();
  ~testee();
  behavior make_behavior() override;
};

testee::testee() {
  // nop
}

testee::~testee() {
  // nop
}

behavior testee::make_behavior() {
  return {
    others >> [=] {
      BOOST_CHECK_EQUAL(current_message().cvals()->get_reference_count(), 2);
      quit();
      return std::move(current_message());
    }
  };
}

class tester : public event_based_actor {
public:
  explicit tester(actor aut)
      : aut_(std::move(aut)),
        msg_(make_message(1, 2, 3)) {
    // nop
  }

  behavior make_behavior() override;
private:
  actor aut_;
  message msg_;
};

behavior tester::make_behavior() {
  monitor(aut_);
  send(aut_, msg_);
  return {
    [=](int a, int b, int c) {
      BOOST_CHECK_EQUAL(a, 1);
      BOOST_CHECK_EQUAL(b, 2);
      BOOST_CHECK_EQUAL(c, 3);
      BOOST_CHECK(current_message().cvals()->get_reference_count() == 2
                && current_message().cvals().get() == msg_.cvals().get());
    },
    [=](const down_msg& dm) {
      BOOST_CHECK(dm.source == aut_
                && dm.reason == exit_reason::normal
                && current_message().cvals()->get_reference_count() == 1);
      quit();
    },
    others >> [&] {
      BOOST_ERROR("Unexpected message: " << to_string(current_message()));
    }
  };
}

template <spawn_options Os>
void test_message_lifetime() {
  // put some preassure on the scheduler (check for thread safety)
  for (size_t i = 0; i < 100; ++i) {
    spawn<tester>(spawn<testee, Os>());
  }
}

struct fixture {
  ~fixture() {
    await_all_actors_done();
    shutdown();
  }
};

} // namespace <anonymous>

BOOST_FIXTURE_TEST_SUITE(message_lifetime_tests, fixture)

BOOST_AUTO_TEST_CASE(message_lifetime_in_scoped_actor) {
  auto msg = make_message(1, 2, 3);
  scoped_actor self;
  self->send(self, msg);
  self->receive(
    [&](int a, int b, int c) {
      BOOST_CHECK_EQUAL(a, 1);
      BOOST_CHECK_EQUAL(b, 2);
      BOOST_CHECK_EQUAL(c, 3);
      BOOST_CHECK_EQUAL(msg.cvals()->get_reference_count(), 2);
      BOOST_CHECK_EQUAL(self->current_message().cvals()->get_reference_count(), 2);
      BOOST_CHECK(self->current_message().cvals().get() == msg.cvals().get());
    }
  );
  BOOST_CHECK_EQUAL(msg.cvals()->get_reference_count(), 1);
  msg = make_message(42);
  self->send(self, msg);
  self->receive(
    [&](int& value) {
      BOOST_CHECK_EQUAL(msg.cvals()->get_reference_count(), 1);
      BOOST_CHECK_EQUAL(self->current_message().cvals()->get_reference_count(), 1);
      BOOST_CHECK(self->current_message().cvals().get() != msg.cvals().get());
      value = 10;
    }
  );
  BOOST_CHECK_EQUAL(msg.get_as<int>(0), 42);
}

BOOST_AUTO_TEST_CASE(message_lifetime_no_spawn_options) {
  test_message_lifetime<no_spawn_options>();
}

BOOST_AUTO_TEST_CASE(message_lifetime_priority_aware) {
  test_message_lifetime<priority_aware>();
}

BOOST_AUTO_TEST_SUITE_END()
