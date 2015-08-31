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

#define BOOST_TEST_MODULE io_dynamic_remote_actor
#include <boost/test/included/unit_test.hpp>

#include <thread>
#include <string>
#include <cstring>
#include <sstream>
#include <iostream>
#include <functional>

#include "boost/actor/all.hpp"
#include "boost/actor/io/all.hpp"

#include "boost/actor/detail/logging.hpp"
#include "boost/actor/detail/singletons.hpp"
#include "boost/actor/detail/run_sub_unit_test.hpp"

#ifdef BOOST_ACTOR_USE_ASIO
#include "boost/actor/io/network/asio_multiplexer.hpp"
#endif // BOOST_ACTOR_USE_ASIO

using namespace std;
using boost::none;
using boost::join;
using boost::variant;
using boost::optional;
using boost::is_any_of;
using boost::token_compress_on;
using namespace boost::actor;

namespace {

using spawn5_done_atom = atom_constant<atom("Spawn5Done")>;
using spawn_ping_atom = atom_constant<atom("SpawnPing")>;
using get_group_atom = atom_constant<atom("GetGroup")>;
using sync_msg_atom = atom_constant<atom("SyncMsg")>;
using ping_ptr_atom = atom_constant<atom("PingPtr")>;
using gclient_atom = atom_constant<atom("GClient")>;
using spawn5_atom = atom_constant<atom("Spawn5")>;
using ping_atom = atom_constant<atom("ping")>;
using pong_atom = atom_constant<atom("pong")>;
using foo_atom = atom_constant<atom("foo")>;
using bar_atom = atom_constant<atom("bar")>;

atomic<long> s_destructors_called;
atomic<long> s_on_exit_called;

constexpr size_t num_pings = 10;

size_t s_pongs = 0;

behavior ping_behavior(local_actor* self, size_t ping_msgs) {
  return {
    [=](pong_atom, int value) -> message {
      if (! self->current_sender()) {
        BOOST_ERROR("current_sender() invalid!");
      }
      BOOST_TEST_MESSAGE("received {'pong', " << value << "}");
      // cout << to_string(self->current_message()) << endl;
      if (++s_pongs >= ping_msgs) {
        BOOST_TEST_MESSAGE("reached maximum, send {'EXIT', user_defined} "
                      << "to last sender and quit with normal reason");
        self->send_exit(self->current_sender(),
                exit_reason::user_shutdown);
        self->quit();
      }
      return make_message(ping_atom::value, value);
    },
    others() >> [=] {
      self->quit(exit_reason::user_shutdown);
    }
  };
}

behavior pong_behavior(local_actor* self) {
  return {
    [](ping_atom, int value)->message {
      return make_message(pong_atom::value, value + 1);
    },
    others() >> [=] {
      self->quit(exit_reason::user_shutdown);
    }
  };
}

size_t pongs() {
  return s_pongs;
}

void event_based_ping(event_based_actor* self, size_t ping_msgs) {
  s_pongs = 0;
  self->become(ping_behavior(self, ping_msgs));
}

void pong(blocking_actor* self, actor ping_actor) {
  self->send(ping_actor, pong_atom::value, 0); // kickoff
  self->receive_loop(pong_behavior(self));
}

using string_pair = std::pair<std::string, std::string>;

using actor_vector = vector<actor>;

void reflector(event_based_actor* self) {
  self->become(others >> [=] {
    BOOST_TEST_MESSAGE("reflect and quit; sender was: "
                << to_string(self->current_sender()));
    self->quit();
    return self->current_message();
  });
}

void spawn5_server_impl(event_based_actor* self, actor client, group grp) {
BOOST_TEST_MESSAGE("this node: " << to_string(detail::singletons::get_node_id()));
BOOST_TEST_MESSAGE("self: " << to_string(self->address()));
  BOOST_CHECK(grp != invalid_group);
  for (int i = 0; i < 2; ++i)
    BOOST_TEST_MESSAGE("spawned local subscriber: "
                << self->spawn_in_group(grp, reflector)->id());
  BOOST_TEST_MESSAGE("send {'Spawn5'} and await {'ok', actor_vector}");
  self->sync_send(client, spawn5_atom::value, grp).then(
    [=](ok_atom, const actor_vector& vec) {
      BOOST_TEST_MESSAGE("received vector with " << vec.size() << " elements");
      auto is_remote = [](const actor& x) -> bool {
        return x.is_remote();
      };
      BOOST_CHECK(std::all_of(vec.begin(), vec.end(), is_remote));
      self->send(grp, "Hello reflectors!", 5.0);
      if (vec.size() != 5) {
        BOOST_TEST_MESSAGE("remote client did not spawn five reflectors!");
      }
      for (auto& a : vec) {
        BOOST_TEST_MESSAGE("monitor actor: " << to_string(a));
        self->monitor(a);
      }
      BOOST_TEST_MESSAGE("wait for reflected messages");
      // receive seven reply messages (2 local, 5 remote)
      auto replies = std::make_shared<int>(0);
      self->become(
        [=](const std::string& x0, double x1) {
          BOOST_TEST_MESSAGE("answer from " << to_string(self->current_sender()));
          BOOST_CHECK_EQUAL(x0, "Hello reflectors!");
          BOOST_CHECK_EQUAL(x1, 5.0);
          if (++*replies == 7) {
            BOOST_TEST_MESSAGE("wait for DOWN messages");
            auto downs = std::make_shared<int>(0);
            self->become(
              [=](const down_msg& dm) {
                if (dm.reason != exit_reason::normal) {
                  BOOST_ERROR("reflector exited for non-normal exit reason!");
                }
                if (++*downs == 5) {
                  BOOST_TEST_MESSAGE("down increased to 5, about to quit");
                  self->send(client, spawn5_done_atom::value);
                  self->quit();
                }
              },
              others >> [=] {
                BOOST_ERROR("Unexpected message: "
                               << to_string(self->current_message()));
                self->quit(exit_reason::user_defined);
              },
              after(chrono::seconds(3)) >> [=] {
                BOOST_ERROR("did only receive " << *downs << " down messages");
                self->quit(exit_reason::user_defined);
              }
            );
          }
        },
        after(std::chrono::seconds(6)) >> [=] {
          BOOST_ERROR("Unexpected timeout");
          self->quit(exit_reason::user_defined);
        }
      );
    },
    others >> [=] {
      BOOST_ERROR("Unexpected message: "
                     << to_string(self->current_message()));
      self->quit(exit_reason::user_defined);
    },
    after(chrono::seconds(10)) >> [=] {
      BOOST_ERROR("Unexpected timeout");
      self->quit(exit_reason::user_defined);
    }
  );
}

// receive seven reply messages (2 local, 5 remote)
void spawn5_server(event_based_actor* self, actor client, bool inverted) {
  BOOST_REQUIRE(client.is_remote());
  BOOST_TEST_MESSAGE("spawn5_server, inverted: " << inverted);
  if (! inverted) {
    spawn5_server_impl(self, client, group::get("local", "foobar"));
  } else {
    BOOST_TEST_MESSAGE("request group");
    self->sync_send(client, get_group_atom::value).then(
      [=](const group& remote_group) {
        BOOST_REQUIRE(self->current_sender() != invalid_actor_addr);
        BOOST_CHECK(self->current_sender()->is_remote());
        BOOST_CHECK(remote_group->is_remote());
        BOOST_TEST_MESSAGE("got group: " << to_string(remote_group)
                    << " from " << to_string(self->current_sender()));
        spawn5_server_impl(self, client, remote_group);
      }
    );
  }
}

void spawn5_client(event_based_actor* self) {
  self->become(
    [](get_group_atom) -> group {
      BOOST_TEST_MESSAGE("received {'GetGroup'}");
      return group::get("local", "foobar");
    },
    [=](spawn5_atom, const group& grp)->message {
      BOOST_TEST_MESSAGE("received {'Spawn5'}");
      actor_vector vec;
      for (int i = 0; i < 5; ++i) {
        vec.push_back(spawn_in_group(grp, reflector));
      }
      BOOST_TEST_MESSAGE("spawned all reflectors");
      return make_message(ok_atom::value, std::move(vec));
    },
    [=](spawn5_done_atom) {
      BOOST_TEST_MESSAGE("received {'Spawn5Done'}");
      self->quit();
    }
  );
}

template <class F>
void await_down(event_based_actor* self, actor ptr, F continuation) {
  self->become(
    [=](const down_msg& dm) -> optional<skip_message_t> {
      if (dm.source == ptr) {
        continuation();
        return none;
      }
      return skip_message(); // not the 'DOWN' message we are waiting for
    }
  );
}

class client : public event_based_actor {
public:
  explicit client(actor server) : server_(std::move(server)) {
    // nop
  }

  behavior make_behavior() override {
    return spawn_ping();
  }

  void on_exit() override {
    ++s_on_exit_called;
  }

  ~client() {
    ++s_destructors_called;
  }

private:
  behavior spawn_ping() {
    BOOST_TEST_MESSAGE("send {'SpawnPing'}");
    send(server_, spawn_ping_atom::value);
    return {
      [=](ping_ptr_atom, const actor& ping) {
        BOOST_TEST_MESSAGE("received ping pointer, spawn pong");
        auto pptr = spawn<monitored + detached + blocking_api>(pong, ping);
        await_down(this, pptr, [=] { send_sync_msg(); });
      }
    };
  }

  void send_sync_msg() {
    BOOST_TEST_MESSAGE("sync send {'SyncMsg', 4.2fSyncMsg}");
    sync_send(server_, sync_msg_atom::value, 4.2f).then(
      [=](ok_atom) {
        send_foobars();
      }
    );
  }

  void send_foobars(int i = 0) {
    if (i == 0) {
      BOOST_TEST_MESSAGE("send foobars");
    }
    if (i == 100)
      test_group_comm();
    else {
      sync_send(server_, foo_atom::value, bar_atom::value, i).then(
        [=](foo_atom, bar_atom, int res) {
          BOOST_CHECK_EQUAL(res, i);
          send_foobars(i + 1);
        }
      );
    }
  }

  void test_group_comm() {
    BOOST_TEST_MESSAGE("test group communication via network");
    sync_send(server_, gclient_atom::value).then(
      [=](gclient_atom, actor gclient) {
        BOOST_TEST_MESSAGE("received " << to_string(current_message()));
        auto s5a = spawn<monitored>(spawn5_server, gclient, false);
        await_down(this, s5a, [=] { test_group_comm_inverted(); });
      }
    );
  }

  void test_group_comm_inverted() {
    BOOST_TEST_MESSAGE("test group communication via network (inverted setup)");
    become(
      [=](gclient_atom) -> message {
        BOOST_TEST_MESSAGE("received `gclient_atom`");
        auto cptr = current_sender();
        auto s5c = spawn<monitored>(spawn5_client);
        // set next behavior
        await_down(this, s5c, [=] {
          BOOST_TEST_MESSAGE("set next behavior");
          quit();
        });
        return make_message(gclient_atom::value, s5c);
      }
    );
  }

  actor server_;
};

class server : public event_based_actor {
public:
  behavior make_behavior() override {
    if (run_in_loop_) {
      trap_exit(true);
    }
    return await_spawn_ping();
  }

  explicit server(bool run_in_loop = false) : run_in_loop_(run_in_loop) {
    // nop
  }

  void on_exit() override {
    ++s_on_exit_called;
  }

  ~server() {
    ++s_destructors_called;
  }

private:
  behavior await_spawn_ping() {
    BOOST_TEST_MESSAGE("await {'SpawnPing'}");
    return {
      [=](spawn_ping_atom) -> message {
        BOOST_TEST_MESSAGE("received {'SpawnPing'}");
        auto client = current_sender();
        if (! client) {
          BOOST_TEST_MESSAGE("last_sender() invalid!");
        }
        BOOST_TEST_MESSAGE("spawn event-based ping actor");
        auto pptr = spawn<monitored>(event_based_ping, num_pings);
        BOOST_TEST_MESSAGE("wait until spawned ping actor is done");
        await_down(this, pptr, [=] {
          BOOST_CHECK_EQUAL(pongs(), num_pings);
          become(await_sync_msg());
        });
        return make_message(ping_ptr_atom::value, pptr);
      },
      [](const exit_msg&) {
        // simply ignored if trap_exit is true
      }
    };
  }

  behavior await_sync_msg() {
    BOOST_TEST_MESSAGE("await {'SyncMsg'}");
    return {
      [=](sync_msg_atom, float f) -> atom_value {
        BOOST_TEST_MESSAGE("received {'SyncMsg', " << f << "}");
        BOOST_CHECK_EQUAL(f, 4.2f);
        become(await_foobars());
        return ok_atom::value;
      },
      [](const exit_msg&) {
        // simply ignored if trap_exit is true
      }
    };
  }

  behavior await_foobars() {
    BOOST_TEST_MESSAGE("await foobars");
    auto foobars = make_shared<int>(0);
    return {
      [=](foo_atom, bar_atom, int i) -> message {
        ++*foobars;
        if (i == 99) {
          BOOST_CHECK_EQUAL(*foobars, 100);
          become(test_group_comm());
        }
        return std::move(current_message());
      },
      [](const exit_msg&) {
        // simply ignored if trap_exit is true
      }
    };
  }

  behavior test_group_comm() {
    BOOST_TEST_MESSAGE("test group communication via network");
    return {
      [=](gclient_atom) -> message {
        BOOST_TEST_MESSAGE("received `gclient_atom`");
        auto cptr = current_sender();
        auto s5c = spawn<monitored>(spawn5_client);
        await_down(this, s5c, [=] {
          BOOST_TEST_MESSAGE("test_group_comm_inverted");
          test_group_comm_inverted(actor_cast<actor>(cptr));
        });
        return make_message(gclient_atom::value, s5c);
      },
      [](const exit_msg&) {
        // simply ignored if trap_exit is true
      }
    };
  }

  void test_group_comm_inverted(actor cptr) {
    BOOST_TEST_MESSAGE("test group communication via network (inverted setup)");
    sync_send(cptr, gclient_atom::value).then(
      [=](gclient_atom, actor gclient) {
        await_down(this, spawn<monitored>(spawn5_server, gclient, true), [=] {
          BOOST_TEST_MESSAGE("`await_down` finished");
          if (! run_in_loop_) {
            quit();
          } else {
            become(await_spawn_ping());
          }
        });
      }
    );
  }

  bool run_in_loop_;
};

void test_remote_actor(const char* path, bool run_remote, bool use_asio) {
  scoped_actor self;
  auto serv = self->spawn<server, monitored>(! run_remote);
  // publish on two distinct ports and use the latter one afterwards
  auto port1 = io::publish(serv, 0, "127.0.0.1");
  BOOST_CHECK(port1 > 0);
  BOOST_TEST_MESSAGE("first publish succeeded on port " << port1);
  auto port2 = io::publish(serv, 0, "127.0.0.1");
  BOOST_CHECK(port2 > 0);
  BOOST_TEST_MESSAGE("second publish succeeded on port " << port2);
  // publish local groups as well
  auto gport = io::publish_local_groups(0);
  BOOST_CHECK(gport > 0);
  // check whether accessing local actors via io::remote_actors works correctly,
  // i.e., does not return a proxy instance
  auto serv2 = io::remote_actor("127.0.0.1", port2);
  BOOST_CHECK(serv2 != invalid_actor && ! serv2->is_remote());
  BOOST_CHECK(serv == serv2);
  thread child;
  if (run_remote) {
    child = detail::run_sub_unit_test(self,
                                      path,
                                      0 /* max runtime dummy */,
                                      BOOST_PP_STRINGIZE(BOOST_TEST_MODULE),
                                      use_asio,
                                      {"--client-port=" + std::to_string(port2),
                                       "--client-port=" + std::to_string(port1),
                                       "--group-port=" + std::to_string(gport)});
  } else {
    BOOST_TEST_MESSAGE("please run client with: "
              << "-c " << port2 << " -c " << port1 << " -g " << gport);
  }
  self->receive(
    [&](const down_msg& dm) {
      BOOST_CHECK(dm.source == serv);
      BOOST_CHECK_EQUAL(dm.reason, exit_reason::normal);
    }
  );
  // wait until separate process (in sep. thread) finished execution
  self->await_all_other_actors_done();
  if (run_remote) {
    child.join();
    self->receive(
      [](const std::string& output) {
        cout << endl << endl << "*** output of client program ***"
             << endl << output << endl;
      }
    );
  }
}

} // namespace <anonymous>

BOOST_AUTO_TEST_CASE(remote_actors) {
  auto argv = boost::unit_test::framework::master_test_suite().argv;
  auto argc = boost::unit_test::framework::master_test_suite().argc;
  // skip everything in argv until the separator "--" is found
  auto cmp = [](const char* x) { return strcmp(x, "--") == 0; };
  auto argv_end = argv + argc;
  auto argv_sep = std::find_if(argv, argv_end, cmp);
  if (argv_sep == argv_end) {
    argc = 0;
  } else {
    argv = argv_sep + 1;
    argc = std::distance(argv, argv_end);
  }
  announce<actor_vector>("actor_vector");
  cout << "this node is: " << to_string(boost::actor::detail::singletons::get_node_id())
       << endl;
  std::vector<uint16_t> ports;
  uint16_t gport = 0;
  auto r = message_builder(argv, argv + argc).extract_opts({
    {"server,s", "run in server mode"},
    {"client-port,c", "add client port (two needed)", ports},
    {"group-port,g", "set group port", gport},
    {"use-asio", "use ASIO network backend (if available)"}
  });
  if (! r.error.empty() || r.opts.count("help") > 0 || ! r.remainder.empty()) {
    cout << r.error << endl << endl << r.helptext << endl;
    return;
  }
  auto use_asio = r.opts.count("use-asio") > 0;
  if (use_asio) {
#   ifdef BOOST_ACTOR_USE_ASIO
    BOOST_TEST_MESSAGE("enable ASIO backend");
    io::set_middleman<io::network::asio_multiplexer>();
#   endif // BOOST_ACTOR_USE_ASIO
  }
  if (r.opts.count("server") > 0) {
    BOOST_TEST_MESSAGE("don't run remote actor (server mode)");
    test_remote_actor(argv[0], false, use_asio);
  } else if (r.opts.count("client-port") > 0) {
    if (ports.size() != 2 || r.opts.count("group-port") == 0) {
      cerr << "*** expected exactly two ports and one group port" << endl
           << endl << r.helptext << endl;
      return;
    }
    scoped_actor self;
    auto serv = io::remote_actor("localhost", ports.front());
    auto serv2 = io::remote_actor("localhost", ports.back());
    // remote_actor is supposed to return the same server
    // when connecting to the same host again
    {
      BOOST_CHECK(serv == io::remote_actor("localhost", ports.front()));
      BOOST_CHECK(serv2 == io::remote_actor("127.0.0.1", ports.back()));
    }
    // connect to published groups
    auto grp = io::remote_group("whatever", "127.0.0.1", gport);
    auto c = self->spawn<client, monitored>(serv);
    self->receive(
      [&](const down_msg& dm) {
        BOOST_CHECK(dm.source == c);
        BOOST_CHECK_EQUAL(dm.reason, exit_reason::normal);
      }
    );
    grp->stop();
  } else {
    for (int i = 0; i < 100; ++i) spawn([]{});
    await_all_actors_done();
    test_remote_actor(boost::unit_test::framework::master_test_suite().argv[0], true, use_asio);
  }
  await_all_actors_done();
  shutdown();
  // we either spawn a server or a client, in both cases
  // there must have been exactly one dtor called
  BOOST_CHECK_EQUAL(s_destructors_called.load(), 1);
  BOOST_CHECK_EQUAL(s_on_exit_called.load(), 1);
}
