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

// exclude this suite; seems to be too much to swallow for MSVC
#ifndef BOOST_ACTOR_WINDOWS

#define BOOST_TEST_MODULE typed_spawn
#include <boost/test/included/unit_test.hpp>

#include <boost/algorithm/string.hpp>

#include "boost/actor/all.hpp"

using namespace std;
using boost::none;
using boost::join;
using boost::variant;
using boost::optional;
using boost::is_any_of;
using boost::token_compress_on;
using namespace boost::actor;

using passed_atom = boost::actor::atom_constant<boost::actor::atom("passed")>;

namespace {

// check invariants of type system
using dummy1 = typed_actor<reacts_to<int, int>,
                           replies_to<double>::with<double>>;

using dummy2 = dummy1::extend<reacts_to<ok_atom>>;

static_assert(std::is_convertible<dummy2, dummy1>::value,
              "handle not assignable to narrower definition");

static_assert(! std::is_convertible<dummy1, dummy2>::value,
              "handle is assignable to broader definition");

/******************************************************************************
 *                        simple request/response test                        *
 ******************************************************************************/

struct my_request {
  int a;
  int b;
};

using server_type = typed_actor<replies_to<my_request>::with<bool>>;

bool operator==(const my_request& lhs, const my_request& rhs) {
  return lhs.a == rhs.a && lhs.b == rhs.b;
}

server_type::behavior_type typed_server1() {
  return {
    [](const my_request& req) {
      return req.a == req.b;
    }
  };
}

server_type::behavior_type typed_server2(server_type::pointer) {
  return typed_server1();
}

class typed_server3 : public server_type::base {

public:

  typed_server3(const string& line, actor buddy) { send(buddy, line); }

  behavior_type make_behavior() override { return typed_server2(this); }

};

void client(event_based_actor* self, actor parent, server_type serv) {
  self->sync_send(serv, my_request{0, 0}).then(
    [=](bool val1) {
      BOOST_CHECK_EQUAL(val1, true);
      self->sync_send(serv, my_request{10, 20}).then(
        [=](bool val2) {
          BOOST_CHECK_EQUAL(val2, false);
          self->send(parent, passed_atom::value);
        }
      );
    }
  );
}

void test_typed_spawn(server_type ts) {
  scoped_actor self;
  self->send(ts, my_request{1, 2});
  self->receive(
    [](bool value) {
      BOOST_CHECK_EQUAL(value, false);
    }
  );
  self->send(ts, my_request{42, 42});
  self->receive(
    [](bool value) {
      BOOST_CHECK_EQUAL(value, true);
    }
  );
  self->sync_send(ts, my_request{10, 20}).await(
    [](bool value) {
      BOOST_CHECK_EQUAL(value, false);
    }
  );
  self->sync_send(ts, my_request{0, 0}).await(
    [](bool value) {
      BOOST_CHECK_EQUAL(value, true);
    }
  );
  self->spawn<monitored>(client, self, ts);
  self->receive(
    [](passed_atom) {
      BOOST_TEST_MESSAGE("received `passed_atom`");
    }
  );
  self->receive(
    [](const down_msg& dmsg) {
      BOOST_CHECK_EQUAL(dmsg.reason, exit_reason::normal);
    }
  );
  self->send_exit(ts, exit_reason::user_shutdown);
}

/******************************************************************************
 *          test skipping of messages intentionally + using become()          *
 ******************************************************************************/

struct get_state_msg {};

using event_testee_type = typed_actor<replies_to<get_state_msg>::with<string>,
                                      replies_to<string>::with<void>,
                                      replies_to<float>::with<void>,
                                      replies_to<int>::with<int>>;

class event_testee : public event_testee_type::base {

public:

  behavior_type wait4string() {
    return {on<get_state_msg>() >> [] { return "wait4string"; },
        on<string>() >> [=] { become(wait4int()); },
        (on<float>() || on<int>()) >> skip_message};
  }

  behavior_type wait4int() {
    return {
      on<get_state_msg>() >> [] { return "wait4int"; },
      on<int>() >> [=]()->int {become(wait4float());
        return 42;
      },
      (on<float>() || on<string>()) >> skip_message
    };
  }

  behavior_type wait4float() {
    return {
      on<get_state_msg>() >> [] {
        return "wait4float";
      },
      on<float>() >> [=] { become(wait4string()); },
      (on<string>() || on<int>()) >> skip_message};
  }

  behavior_type make_behavior() override {
    return wait4int();
  }

};

/******************************************************************************
 *                         simple 'forwarding' chain                          *
 ******************************************************************************/

using string_actor = typed_actor<replies_to<string>::with<string>>;

string_actor::behavior_type string_reverter() {
  return {
    [](string& str) {
      std::reverse(str.begin(), str.end());
      return std::move(str);
    }
  };
}

// uses `return sync_send(...).then(...)`
string_actor::behavior_type string_relay(string_actor::pointer self,
                                         string_actor master, bool leaf) {
  auto next = leaf ? spawn(string_relay, master, false) : master;
  self->link_to(next);
  return {
    [=](const string& str) {
      return self->sync_send(next, str).then(
        [](string& answer) -> string {
          return std::move(answer);
        }
      );
    }
  };
}

// uses `return delegate(...)`
string_actor::behavior_type string_delegator(string_actor::pointer self,
                                             string_actor master, bool leaf) {
  auto next = leaf ? spawn(string_delegator, master, false) : master;
  self->link_to(next);
  return {
    [=](string& str) -> delegated<string> {
      return self->delegate(next, std::move(str));
    }
  };
}

using maybe_string_actor = typed_actor<replies_to<string>
                                       ::with_either<ok_atom, string>
                                       ::or_else<error_atom>>;

maybe_string_actor::behavior_type maybe_string_reverter() {
  return {
    [](string& str) -> either<ok_atom, string>::or_else<error_atom> {
      if (str.empty())
        return {error_atom::value};
      std::reverse(str.begin(), str.end());
      return {ok_atom::value, std::move(str)};
    }
  };
}

maybe_string_actor::behavior_type
maybe_string_delegator(maybe_string_actor::pointer self, maybe_string_actor x) {
  self->link_to(x);
  return {
    [=](string& s) -> delegated<either<ok_atom, string>::or_else<error_atom>> {
      return self->delegate(x, std::move(s));
    }
  };
}

/******************************************************************************
 *                        sending typed actor handles                         *
 ******************************************************************************/

using int_actor = typed_actor<replies_to<int>::with<int>>;

int_actor::behavior_type int_fun() {
  return {
    [](int i) { return i * i; }
  };
}

behavior foo(event_based_actor* self) {
  return {
    [=](int i, int_actor server) {
      return self->sync_send(server, i).then([=](int result) -> int {
        self->quit(exit_reason::normal);
        return result;
      });
    }
  };
}

int_actor::behavior_type int_fun2(int_actor::pointer self) {
  self->trap_exit(true);
  return {
    [=](int i) {
      self->monitor(self->current_sender());
      return i * i;
    },
    [=](const down_msg& dm) {
      BOOST_CHECK_EQUAL(dm.reason, exit_reason::normal);
      self->quit();
    },
    [=](const exit_msg&) {
      BOOST_ERROR("Unexpected message: "
                     << to_string(self->current_message()));
    }
  };
}

behavior foo2(event_based_actor* self) {
  return {
    [=](int i, int_actor server) {
      return self->sync_send(server, i).then([=](int result) -> int {
        self->quit(exit_reason::normal);
        return result;
      });
    }
  };
}

struct fixture {
  fixture() {
    announce<get_state_msg>("get_state_msg");
    announce<int_actor>("int_actor");
    announce<my_request>("my_request", &my_request::a, &my_request::b);
  }

  ~fixture() {
    await_all_actors_done();
    shutdown();
  }
};

} // namespace <anonymous>

BOOST_FIXTURE_TEST_SUITE(typed_spawn_tests, fixture)

/******************************************************************************
 *                             put it all together                            *
 ******************************************************************************/

BOOST_AUTO_TEST_CASE(typed_spawns) {
  // run test series with typed_server(1|2)
  test_typed_spawn(spawn(typed_server1));
  await_all_actors_done();
  BOOST_TEST_MESSAGE("finished test series with `typed_server1`");

  test_typed_spawn(spawn(typed_server2));
  await_all_actors_done();
  BOOST_TEST_MESSAGE("finished test series with `typed_server2`");
  {
    scoped_actor self;
    test_typed_spawn(spawn<typed_server3>("hi there", self));
    self->receive(on("hi there") >> [] {
      BOOST_TEST_MESSAGE("received \"hi there\"");
    });
  }
}

BOOST_AUTO_TEST_CASE(test_event_testee) {
  // run test series with event_testee
  scoped_actor self;
  auto et = self->spawn<event_testee>();
  string result;
  self->send(et, 1);
  self->send(et, 2);
  self->send(et, 3);
  self->send(et, .1f);
  self->send(et, "hello event testee!");
  self->send(et, .2f);
  self->send(et, .3f);
  self->send(et, "hello again event testee!");
  self->send(et, "goodbye event testee!");
  typed_actor<replies_to<get_state_msg>::with<string>> sub_et = et;
  // $:: is the anonymous namespace
  set<string> iface{"boost::actor::replies_to<get_state_msg>::with<@str>",
                    "boost::actor::replies_to<@str>::with<void>",
                    "boost::actor::replies_to<float>::with<void>",
                    "boost::actor::replies_to<@i32>::with<@i32>"};
  BOOST_CHECK_EQUAL(join(sub_et->message_types(), ","), join(iface, ","));
  self->send(sub_et, get_state_msg{});
  // we expect three 42s
  int i = 0;
  self->receive_for(i, 3)([](int value) { BOOST_CHECK_EQUAL(value, 42); });
  self->receive(
    [&](const string& str) {
      result = str;
    },
    after(chrono::minutes(1)) >> [&] {
      BOOST_ERROR("event_testee does not reply");
      throw runtime_error("event_testee does not reply");
    }
  );
  self->send_exit(et, exit_reason::user_shutdown);
  self->await_all_other_actors_done();
  BOOST_CHECK_EQUAL(result, "wait4int");
}

BOOST_AUTO_TEST_CASE(reverter_relay_chain) {
  // run test series with string reverter
  scoped_actor self;
  // actor-under-test
  auto aut = self->spawn<monitored>(string_relay,
                                          spawn(string_reverter),
                                          true);
  set<string> iface{"boost::actor::replies_to<@str>::with<@str>"};
  BOOST_CHECK(aut->message_types() == iface);
  self->sync_send(aut, "Hello World!").await(
    [](const string& answer) {
      BOOST_CHECK_EQUAL(answer, "!dlroW olleH");
    }
  );
  anon_send_exit(aut, exit_reason::user_shutdown);
}

BOOST_AUTO_TEST_CASE(string_delegator_chain) {
  // run test series with string reverter
  scoped_actor self;
  // actor-under-test
  auto aut = self->spawn<monitored>(string_delegator,
                                          spawn(string_reverter),
                                          true);
  set<string> iface{"boost::actor::replies_to<@str>::with<@str>"};
  BOOST_CHECK(aut->message_types() == iface);
  self->sync_send(aut, "Hello World!").await(
    [](const string& answer) {
      BOOST_CHECK_EQUAL(answer, "!dlroW olleH");
    }
  );
  anon_send_exit(aut, exit_reason::user_shutdown);
}

BOOST_AUTO_TEST_CASE(maybe_string_delegator_chain) {
  scoped_actor self;
  auto aut = spawn(maybe_string_delegator,
                   spawn(maybe_string_reverter));
  self->sync_send(aut, "").await(
    [](ok_atom, const string&) {
      throw std::logic_error("unexpected result!");
    },
    [](error_atom) {
      // nop
    }
  );
  self->sync_send(aut, "abcd").await(
    [](ok_atom, const string& str) {
      BOOST_CHECK_EQUAL(str, "dcba");
    },
    [](error_atom) {
      throw std::logic_error("unexpected error_atom!");
    }
  );
  anon_send_exit(aut, exit_reason::user_shutdown);
}

BOOST_AUTO_TEST_CASE(test_sending_typed_actors) {
  scoped_actor self;
  auto aut = spawn(int_fun);
  self->send(spawn(foo), 10, aut);
  self->receive(
    [](int i) { BOOST_CHECK_EQUAL(i, 100); }
  );
  self->send_exit(aut, exit_reason::user_shutdown);
}

BOOST_AUTO_TEST_CASE(test_sending_typed_actors_and_down_msg) {
  scoped_actor self;
  auto aut = spawn(int_fun2);
  self->send(spawn(foo2), 10, aut);
  self->receive([](int i) { BOOST_CHECK_EQUAL(i, 100); });
}

BOOST_AUTO_TEST_CASE(check_signature) {
  using foo_type = typed_actor<replies_to<put_atom>::
                               with_either<ok_atom>::or_else<error_atom>>;
  using foo_result_type = either<ok_atom>::or_else<error_atom>;
  using bar_type = typed_actor<reacts_to<ok_atom>, reacts_to<error_atom>>;
  auto foo_action = [](foo_type::pointer self) -> foo_type::behavior_type {
    return {
      [=] (put_atom) -> foo_result_type {
        self->quit();
        return {ok_atom::value};
      }
    };
  };
  auto bar_action = [=](bar_type::pointer self) -> bar_type::behavior_type {
    auto foo = self->spawn<linked>(foo_action);
    self->send(foo, put_atom::value);
    return {
      [=](ok_atom) {
        self->quit();
      },
      [=](error_atom) {
        self->quit(exit_reason::user_defined);
      }
    };
  };
  scoped_actor self;
  self->spawn<monitored>(bar_action);
  self->receive(
    [](const down_msg& dm) {
      BOOST_CHECK_EQUAL(dm.reason, exit_reason::normal);
    }
  );
}

BOOST_AUTO_TEST_SUITE_END()

#endif // BOOST_ACTOR_WINDOWS
