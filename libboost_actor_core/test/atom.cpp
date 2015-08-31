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

#define BOOST_TEST_MODULE atom
#include <boost/test/included/unit_test.hpp>

#include <string>

#include "boost/actor/all.hpp"
#include "boost/actor/shutdown.hpp"

using boost::none;
using boost::join;
using boost::variant;
using boost::optional;
using boost::is_any_of;
using boost::token_compress_on;
using namespace boost::actor;

namespace {

constexpr auto s_foo = atom("FooBar");

using a_atom = atom_constant<atom("a")>;
using b_atom = atom_constant<atom("b")>;
using c_atom = atom_constant<atom("c")>;
using abc_atom = atom_constant<atom("abc")>;
using def_atom = atom_constant<atom("def")>;
using foo_atom = atom_constant<atom("foo")>;

struct fixture {
  ~fixture() {
    await_all_actors_done();
    shutdown();
  }
};

} // namespace <anonymous>

BOOST_FIXTURE_TEST_SUITE(atom_tests, fixture)

BOOST_AUTO_TEST_CASE(basics) {
  // check if there are leading bits that distinguish "zzz" and "000 "
  BOOST_CHECK(atom("zzz") != atom("000 "));
  // check if there are leading bits that distinguish "abc" and " abc"
  BOOST_CHECK(atom("abc") != atom(" abc"));
  // 'illegal' characters are mapped to whitespaces
  BOOST_CHECK(atom("   ") == atom("@!?"));
  // check to_string impl
  BOOST_CHECK_EQUAL(to_string(s_foo), "FooBar");
}

struct send_to_self {
  explicit send_to_self(blocking_actor* self) : self_(self) {
    // nop
  }
  template <class... Ts>
  void operator()(Ts&&... xs) {
    self_->send(self_, std::forward<Ts>(xs)...);
  }
  blocking_actor* self_;
};

BOOST_AUTO_TEST_CASE(receive_atoms) {
  scoped_actor self;
  send_to_self f{self.get()};
  f(foo_atom::value, static_cast<uint32_t>(42));
  f(abc_atom::value, def_atom::value, "cstring");
  f(1.f);
  f(a_atom::value, b_atom::value, c_atom::value, 23.f);
  bool matched_pattern[3] = {false, false, false};
  int i = 0;
  BOOST_TEST_MESSAGE("start receive loop");
  for (i = 0; i < 3; ++i) {
    self->receive(
      [&](foo_atom, uint32_t value) {
        matched_pattern[0] = true;
        BOOST_CHECK_EQUAL(value, 42);
      },
      [&](abc_atom, def_atom, const std::string& str) {
        matched_pattern[1] = true;
        BOOST_CHECK_EQUAL(str, "cstring");
      },
      [&](a_atom, b_atom, c_atom, float value) {
        matched_pattern[2] = true;
        BOOST_CHECK_EQUAL(value, 23.f);
      });
  }
  BOOST_CHECK(matched_pattern[0] && matched_pattern[1] && matched_pattern[2]);
  self->receive(
    // "erase" message { 'b', 'a, 'c', 23.f }
    others >> [] {
      BOOST_TEST_MESSAGE("drain mailbox");
    },
    after(std::chrono::seconds(0)) >> [] {
      BOOST_ERROR("mailbox empty");
    }
  );
  atom_value x = atom("abc");
  atom_value y = abc_atom::value;
  BOOST_CHECK(x == y);
  auto msg = make_message(atom("abc"));
  self->send(self, msg);
  self->receive(
    [](abc_atom) {
      BOOST_TEST_MESSAGE("received 'abc'");
    },
    others >> [] {
      BOOST_ERROR("unexpected message");
    }
  );
}

using testee = typed_actor<replies_to<abc_atom>::with<int>>;

testee::behavior_type testee_impl(testee::pointer self) {
  return {
    [=](abc_atom) {
      self->quit();
      return 42;
    }
  };
}

BOOST_AUTO_TEST_CASE(sync_send_atom_constants) {
  scoped_actor self;
  auto tst = spawn(testee_impl);
  self->sync_send(tst, abc_atom::value).await(
    [](int i) {
      BOOST_CHECK_EQUAL(i, 42);
    }
  );
}

BOOST_AUTO_TEST_SUITE_END()
