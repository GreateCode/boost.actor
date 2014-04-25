#include <stack>
#include <chrono>
#include <iostream>
#include <functional>

#include "test.hpp"
#include "ping_pong.hpp"

#include "boost/actor/cppa.hpp"

#include "boost/actor/detail/cs_thread.hpp"
#include "boost/actor/detail/yield_interface.hpp"

using namespace std;
using namespace boost::actor;

namespace {

class event_testee : public sb_actor<event_testee> {

    friend class sb_actor<event_testee>;

    behavior wait4string;
    behavior wait4float;
    behavior wait4int;

    behavior& init_state = wait4int;

 public:

    event_testee() {
        wait4string = (
            on<string>() >> [=] {
                become(wait4int);
            },
            on<atom("get_state")>() >> [=] {
                return "wait4string";
            }
        );

        wait4float = (
            on<float>() >> [=] {
                become(wait4string);
            },
            on<atom("get_state")>() >> [=] {
                return "wait4float";
            }
        );

        wait4int = (
            on<int>() >> [=] {
                become(wait4float);
            },
            on<atom("get_state")>() >> [=] {
                return "wait4int";
            }
        );
    }

};

// quits after 5 timeouts
actor spawn_event_testee2(actor parent) {
    struct impl : event_based_actor {
        actor parent;
        impl(actor parent_actor) : parent(parent_actor) { }
        behavior wait4timeout(int remaining) {
            BOOST_ACTOR_LOG_TRACE(BOOST_ACTOR_ARG(remaining));
            return {
                after(chrono::milliseconds(1)) >> [=] {
                    BOOST_ACTOR_PRINT(BOOST_ACTOR_ARG(remaining));
                    if (remaining == 1) {
                        send(parent, atom("t2done"));
                        quit();
                    }
                    else become(wait4timeout(remaining - 1));
                }
            };
        }
        behavior make_behavior() override {
            return wait4timeout(5);
        }
    };
    return spawn<impl>(parent);
}

struct chopstick : public sb_actor<chopstick> {

    behavior taken_by(actor whom) {
        return (
            on<atom("take")>() >> [=] {
                return atom("busy");
            },
            on(atom("put"), whom) >> [=] {
                become(available);
            },
            on(atom("break")) >> [=] {
                quit();
            }
        );
    }

    behavior available;

    behavior& init_state = available;

    chopstick() {
        available = (
            on(atom("take"), arg_match) >> [=](actor whom) -> atom_value {
                become(taken_by(whom));
                return atom("taken");
            },
            on(atom("break")) >> [=] {
                quit();
            }
        );
    }

};

class testee_actor {

    void wait4string(blocking_actor* self) {
        bool string_received = false;
        self->do_receive (
            on<string>() >> [&] {
                string_received = true;
            },
            on<atom("get_state")>() >> [&] {
                return "wait4string";
            }
        )
        .until(gref(string_received));
    }

    void wait4float(blocking_actor* self) {
        bool float_received = false;
        self->do_receive (
            on<float>() >> [&] {
                float_received = true;
            },
            on<atom("get_state")>() >> [&] {
                return "wait4float";
            }
        )
        .until(gref(float_received));
        wait4string(self);
    }

 public:

    void operator()(blocking_actor* self) {
        self->receive_loop (
            on<int>() >> [&] {
                wait4float(self);
            },
            on<atom("get_state")>() >> [&] {
                return "wait4int";
            }
        );
    }

};

// self->receives one timeout and quits
void testee1(event_based_actor* self) {
    BOOST_ACTOR_LOGF_TRACE("");
    self->become(after(chrono::milliseconds(10)) >> [=] {
        BOOST_ACTOR_LOGF_TRACE("");
        self->unbecome();
    });
}

void testee2(event_based_actor* self, actor other) {
    self->link_to(other);
    self->send(other, uint32_t(1));
    self->become (
        on<uint32_t>() >> [=](uint32_t sleep_time) {
            // "sleep" for sleep_time milliseconds
            self->become (
                keep_behavior,
                after(chrono::milliseconds(sleep_time)) >> [=] {
                    self->unbecome();
                }
            );
        }
    );
}

template<class Testee>
string behavior_test(scoped_actor& self, actor et) {
    string testee_name = detail::to_uniform_name(typeid(Testee));
    BOOST_ACTOR_LOGF_TRACE(BOOST_ACTOR_TARG(et, to_string) << ", " << BOOST_ACTOR_ARG(testee_name));
    string result;
    self->send(et, 1);
    self->send(et, 2);
    self->send(et, 3);
    self->send(et, .1f);
    self->send(et, "hello " + testee_name);
    self->send(et, .2f);
    self->send(et, .3f);
    self->send(et, "hello again " + testee_name);
    self->send(et, "goodbye " + testee_name);
    self->send(et, atom("get_state"));
    self->receive (
        on_arg_match >> [&](const string& str) {
            result = str;
        },
        after(chrono::minutes(1)) >> [&]() {
            BOOST_ACTOR_LOGF_ERROR(testee_name << " does not reply");
            throw runtime_error(testee_name + " does not reply");
        }
    );
    self->send_exit(et, exit_reason::user_shutdown);
    self->await_all_other_actors_done();
    return result;
}

class fixed_stack : public sb_actor<fixed_stack> {

    friend class sb_actor<fixed_stack>;

    size_t max_size = 10;

    vector<int> data;

    behavior full;
    behavior filled;
    behavior empty;

    behavior& init_state = empty;

 public:

    fixed_stack(size_t max) : max_size(max)  {
        full = (
            on(atom("push"), arg_match) >> [=](int) { /* discard */ },
            on(atom("pop")) >> [=]() -> std::tuple<atom_value, int> {
                auto result = data.back();
                data.pop_back();
                become(filled);
                return {atom("ok"), result};
            }
        );
        filled = (
            on(atom("push"), arg_match) >> [=](int what) {
                data.push_back(what);
                if (data.size() == max_size) become(full);
            },
            on(atom("pop")) >> [=]() -> std::tuple<atom_value, int> {
                auto result = data.back();
                data.pop_back();
                if (data.empty()) become(empty);
                return {atom("ok"), result};
            }
        );
        empty = (
            on(atom("push"), arg_match) >> [=](int what) {
                data.push_back(what);
                become(filled);
            },
            on(atom("pop")) >> [=] {
                return atom("failure");
            }
        );

    }

};

behavior echo_actor(event_based_actor* self) {
    return (
        others() >> [=]() -> any_tuple {
            self->quit(exit_reason::normal);
            return self->last_dequeued();
        }
    );
}


struct simple_mirror : sb_actor<simple_mirror> {

    behavior init_state;

    simple_mirror() {
        init_state = (
            others() >> [=]() -> any_tuple {
                return last_dequeued();
            }
        );
    }

};

behavior high_priority_testee(event_based_actor* self) {
    self->send(self, atom("b"));
    self->send(message_priority::high, self, atom("a"));
    // 'a' must be self->received before 'b'
    return (
        on(atom("b")) >> [=] {
            BOOST_ACTOR_FAILURE("received 'b' before 'a'");
            self->quit();
        },
        on(atom("a")) >> [=] {
            BOOST_ACTOR_CHECKPOINT();
            self->become (
                on(atom("b")) >> [=] {
                    BOOST_ACTOR_CHECKPOINT();
                    self->quit();
                },
                others() >> BOOST_ACTOR_UNEXPECTED_MSG_CB(self)
            );
        },
        others() >> BOOST_ACTOR_UNEXPECTED_MSG_CB(self)
    );
}

struct high_priority_testee_class : event_based_actor {
    behavior make_behavior() override {
        return high_priority_testee(this);
    }
};

struct master : event_based_actor {
    behavior make_behavior() override {
        return (
            on(atom("done")) >> [=] {
                quit(exit_reason::user_shutdown);
            }
        );
    }
};

struct slave : event_based_actor {

    slave(actor master_actor) : master{master_actor} { }

    behavior make_behavior() override {
        link_to(master);
        return (
            others() >> BOOST_ACTOR_UNEXPECTED_MSG_CB(this)
        );
    }

    actor master;

};

void test_serial_reply() {
    auto mirror_behavior = [=](event_based_actor* self) {
        self->become(others() >> [=]() -> any_tuple {
            BOOST_ACTOR_PRINT("return self->last_dequeued()");
            return self->last_dequeued();
        });
    };
    auto master = spawn([=](event_based_actor* self) {
        cout << "ID of master: " << self->id() << endl;
        // spawn 5 mirror actors
        auto c0 = self->spawn<linked>(mirror_behavior);
        auto c1 = self->spawn<linked>(mirror_behavior);
        auto c2 = self->spawn<linked>(mirror_behavior);
        auto c3 = self->spawn<linked>(mirror_behavior);
        auto c4 = self->spawn<linked>(mirror_behavior);
        self->become (
          on(atom("hi there")) >> [=]() -> continue_helper {
            BOOST_ACTOR_PRINT("received 'hi there'");
            return self->sync_send(c0, atom("sub0")).then(
              on(atom("sub0")) >> [=]() -> continue_helper {
                BOOST_ACTOR_PRINT("received 'sub0'");
                return self->sync_send(c1, atom("sub1")).then(
                  on(atom("sub1")) >> [=]() -> continue_helper {
                    BOOST_ACTOR_PRINT("received 'sub1'");
                    return self->sync_send(c2, atom("sub2")).then(
                      on(atom("sub2")) >> [=]() -> continue_helper {
                        BOOST_ACTOR_PRINT("received 'sub2'");
                        return self->sync_send(c3, atom("sub3")).then(
                          on(atom("sub3")) >> [=]() -> continue_helper {
                            BOOST_ACTOR_PRINT("received 'sub3'");
                            return self->sync_send(c4, atom("sub4")).then(
                              on(atom("sub4")) >> [=]() -> atom_value {
                                BOOST_ACTOR_PRINT("received 'sub4'");
                                return atom("hiho");
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
      }
    );
    { // lifetime scope of self
        scoped_actor self;
        cout << "ID of main: " << self->id() << endl;
        self->sync_send(master, atom("hi there")).await(
            on(atom("hiho")) >> [] {
                BOOST_ACTOR_CHECKPOINT();
            },
            others() >> BOOST_ACTOR_UNEXPECTED_MSG_CB_REF(self)
        );
        self->send_exit(master, exit_reason::user_shutdown);
    }
    await_all_actors_done();
}

void test_or_else() {
    scoped_actor self;
    partial_function handle_a {
        on("a") >> [] { return 1; }
    };
    partial_function handle_b {
        on("b") >> [] { return 2; }
    };
    partial_function handle_c {
        on("c") >> [] { return 3; }
    };
    auto run_testee([&](actor testee) {
        self->sync_send(testee, "a").await([](int i) {
            BOOST_ACTOR_CHECK_EQUAL(i, 1);
        });
        self->sync_send(testee, "b").await([](int i) {
            BOOST_ACTOR_CHECK_EQUAL(i, 2);
        });
        self->sync_send(testee, "c").await([](int i) {
            BOOST_ACTOR_CHECK_EQUAL(i, 3);
        });
        self->send_exit(testee, exit_reason::user_shutdown);
        self->await_all_other_actors_done();

    });
    BOOST_ACTOR_PRINT("run_testee: handle_a.or_else(handle_b).or_else(handle_c)");
    run_testee(
        spawn([=] {
            return handle_a.or_else(handle_b).or_else(handle_c);
        })
    );
    BOOST_ACTOR_PRINT("run_testee: handle_a.or_else(handle_b), on(\"c\") ...");
    run_testee(
        spawn([=]() -> behavior {
            return {
                handle_a.or_else(handle_b),
                on("c") >> [] { return 3; }
            };
        })
    );
    BOOST_ACTOR_PRINT("run_testee: on(\"a\") ..., handle_b.or_else(handle_c)");
    run_testee(
        spawn([=]() -> behavior {
            return {
                on("a") >> [] { return 1; },
                handle_b.or_else(handle_c)
            };
        })
    );
}

void test_continuation() {
    auto mirror = spawn<simple_mirror>();
    spawn([=](event_based_actor* self) {
        self->sync_send(mirror, 42).then(
            on(42) >> [] {
                return "fourty-two";
            }
        ).continue_with(
            [=](const string& ref) {
                BOOST_ACTOR_CHECK_EQUAL(ref, "fourty-two");
                return 4.2f;
            }
        ).continue_with(
            [=](float f) {
                BOOST_ACTOR_CHECK_EQUAL(f, 4.2f);
                self->send_exit(mirror, exit_reason::user_shutdown);
                self->quit();
            }
        );
    });
    await_all_actors_done();
}

void test_simple_reply_response() {
    auto s = spawn([](event_based_actor* self) -> behavior {
        return (
            others() >> [=]() -> any_tuple {
                BOOST_ACTOR_CHECK(self->last_dequeued() == make_any_tuple(atom("hello")));
                self->quit();
                return self->last_dequeued();
            }
        );
    });
    scoped_actor self;
    self->send(s, atom("hello"));
    self->receive(
        others() >> [&] {
            BOOST_ACTOR_CHECK(self->last_dequeued() == make_any_tuple(atom("hello")));
        }
    );
    self->await_all_other_actors_done();
}

void test_spawn() {
    test_simple_reply_response();
    BOOST_ACTOR_CHECKPOINT();
    test_serial_reply();
    BOOST_ACTOR_CHECKPOINT();
    test_or_else();
    BOOST_ACTOR_CHECKPOINT();
    test_continuation();
    BOOST_ACTOR_CHECKPOINT();
    scoped_actor self;
    // check whether detached actors and scheduled actors interact w/o errors
    auto m = spawn<master, detached>();
    spawn<slave>(m);
    spawn<slave>(m);
    self->send(m, atom("done"));
    self->await_all_other_actors_done();
    BOOST_ACTOR_CHECKPOINT();

    BOOST_ACTOR_PRINT("test self->send()");
    self->send(self, 1, 2, 3, true);
    self->receive(on(1, 2, 3, true) >> [] { });
    self->send_tuple(self, any_tuple{});
    self->receive(on() >> [] { });
    self->await_all_other_actors_done();
    BOOST_ACTOR_CHECKPOINT();

    BOOST_ACTOR_PRINT("test self->receive with zero timeout");
    self->receive (
        others() >> BOOST_ACTOR_UNEXPECTED_MSG_CB_REF(self),
        after(chrono::seconds(0)) >> [] { /* mailbox empty */ }
    );
    self->await_all_other_actors_done();
    BOOST_ACTOR_CHECKPOINT();

    BOOST_ACTOR_PRINT("test mirror"); {
        auto mirror = self->spawn<simple_mirror, monitored>();
        self->send(mirror, "hello mirror");
        self->receive (
            on("hello mirror") >> BOOST_ACTOR_CHECKPOINT_CB(),
            others() >> BOOST_ACTOR_UNEXPECTED_MSG_CB_REF(self)
        );
        self->send_exit(mirror, exit_reason::user_shutdown);
        self->receive (
            on_arg_match >> [&](const down_msg& dm) {
                if (dm.reason == exit_reason::user_shutdown) {
                    BOOST_ACTOR_CHECKPOINT();
                }
                else { BOOST_ACTOR_UNEXPECTED_MSG_CB_REF(self); }
            },
            others() >> BOOST_ACTOR_UNEXPECTED_MSG_CB_REF(self)
        );
        self->await_all_other_actors_done();
        BOOST_ACTOR_CHECKPOINT();
    }

    BOOST_ACTOR_PRINT("test detached mirror"); {
        auto mirror = self->spawn<simple_mirror, monitored+detached>();
        self->send(mirror, "hello mirror");
        self->receive (
            on("hello mirror") >> BOOST_ACTOR_CHECKPOINT_CB(),
            others() >> BOOST_ACTOR_UNEXPECTED_MSG_CB_REF(self)
        );
        self->send_exit(mirror, exit_reason::user_shutdown);
        self->receive (
            on_arg_match >> [&](const down_msg& dm) {
                if (dm.reason == exit_reason::user_shutdown) {
                    BOOST_ACTOR_CHECKPOINT();
                }
                else { BOOST_ACTOR_UNEXPECTED_MSG(self); }
            },
            others() >> BOOST_ACTOR_UNEXPECTED_MSG_CB_REF(self)
        );
        self->await_all_other_actors_done();
        BOOST_ACTOR_CHECKPOINT();
    }

    BOOST_ACTOR_PRINT("test priority aware mirror"); {
        auto mirror = self->spawn<simple_mirror, monitored+priority_aware>();
        BOOST_ACTOR_CHECKPOINT();
        self->send(mirror, "hello mirror");
        self->receive (
            on("hello mirror") >> BOOST_ACTOR_CHECKPOINT_CB(),
            others() >> BOOST_ACTOR_UNEXPECTED_MSG_CB_REF(self)
        );
        self->send_exit(mirror, exit_reason::user_shutdown);
        self->receive (
            on_arg_match >> [&](const down_msg& dm) {
                if (dm.reason == exit_reason::user_shutdown) {
                    BOOST_ACTOR_CHECKPOINT();
                }
                else { BOOST_ACTOR_UNEXPECTED_MSG(self); }
            },
            others() >> BOOST_ACTOR_UNEXPECTED_MSG_CB_REF(self)
        );
        self->await_all_other_actors_done();
        BOOST_ACTOR_CHECKPOINT();
    }

    BOOST_ACTOR_PRINT("test echo actor");
    auto mecho = spawn(echo_actor);
    self->send(mecho, "hello echo");
    self->receive (
        on("hello echo") >> [] { },
        others() >> BOOST_ACTOR_UNEXPECTED_MSG_CB_REF(self)
    );
    self->await_all_other_actors_done();
    BOOST_ACTOR_CHECKPOINT();

    BOOST_ACTOR_PRINT("test timeout");
    self->receive(after(chrono::seconds(1)) >> [] { });
    BOOST_ACTOR_CHECKPOINT();

    BOOST_ACTOR_PRINT("test delayed_send()");
    self->delayed_send(self, chrono::seconds(1), 1, 2, 3);
    self->receive(on(1, 2, 3) >> [] { });
    self->await_all_other_actors_done();
    BOOST_ACTOR_CHECKPOINT();

    spawn(testee1);
    self->await_all_other_actors_done();
    BOOST_ACTOR_CHECKPOINT();

    spawn_event_testee2(self);
    self->receive(on(atom("t2done")) >> BOOST_ACTOR_CHECKPOINT_CB());
    self->await_all_other_actors_done();
    BOOST_ACTOR_CHECKPOINT();

    auto cstk = spawn<chopstick>();
    self->send(cstk, atom("take"), self);
    self->receive (
        on(atom("taken")) >> [&] {
            self->send(cstk, atom("put"), self);
            self->send(cstk, atom("break"));
        },
        others() >> BOOST_ACTOR_UNEXPECTED_MSG_CB_REF(self)
    );
    self->await_all_other_actors_done();
    BOOST_ACTOR_CHECKPOINT();

    auto st = spawn<fixed_stack>(size_t{10});
    // push 20 values
    for (int i = 0; i < 20; ++i) self->send(st, atom("push"), i);
    // pop 20 times
    for (int i = 0; i < 20; ++i) self->send(st, atom("pop"));
    // expect 10 failure messages
    {
        int i = 0;
        self->receive_for(i, 10) (
            on(atom("failure")) >> BOOST_ACTOR_CHECKPOINT_CB()
        );
        BOOST_ACTOR_CHECKPOINT();
    }
    // expect 10 {'ok', value} messages
    {
        vector<int> values;
        int i = 0;
        self->receive_for(i, 10) (
            on(atom("ok"), arg_match) >> [&](int value) {
                values.push_back(value);
            }
        );
        vector<int> expected{9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
        BOOST_ACTOR_CHECK(values == expected);
    }
    // terminate st
    self->send_exit(st, exit_reason::user_shutdown);
    self->await_all_other_actors_done();
    BOOST_ACTOR_CHECKPOINT();

    auto sync_testee1 = spawn<blocking_api>([](blocking_actor* s) {
        if (detail::cs_thread::is_disabled_feature) {
            BOOST_ACTOR_LOGF_WARNING("compiled w/o context switching "
                              "(skip some tests)");
        }
        else {
            BOOST_ACTOR_CHECKPOINT();
            // scheduler should switch back immediately
            detail::yield(detail::yield_state::ready);
            BOOST_ACTOR_CHECKPOINT();
        }
        s->receive (
            on(atom("get")) >> [] {
                return std::make_tuple(42, 2);
            }
        );
    });
    self->send(self, 0, 0);
    auto handle = self->sync_send(sync_testee1, atom("get"));
    // wait for some time (until sync response arrived in mailbox)
    self->receive (after(chrono::milliseconds(50)) >> [] { });
    // enqueue async messages (must be skipped by self->receive_response)
    self->send(self, 42, 1);
    // must skip sync message
    self->receive (
        on(42, arg_match) >> [&](int i) {
            BOOST_ACTOR_CHECK_EQUAL(i, 1);
        }
    );
    // must skip remaining async message
    handle.await(
        on_arg_match >> [&](int a, int b) {
            BOOST_ACTOR_CHECK_EQUAL(a, 42);
            BOOST_ACTOR_CHECK_EQUAL(b, 2);
        },
        others() >> BOOST_ACTOR_UNEXPECTED_MSG_CB_REF(self),
        after(chrono::seconds(10)) >> BOOST_ACTOR_UNEXPECTED_TOUT_CB()
    );
    // dequeue remaining async. message
    self->receive (on(0, 0) >> BOOST_ACTOR_CHECKPOINT_CB());
    // make sure there's no other message in our mailbox
    self->receive (
        others() >> BOOST_ACTOR_UNEXPECTED_MSG_CB_REF(self),
        after(chrono::seconds(0)) >> [] { }
    );
    self->await_all_other_actors_done();
    BOOST_ACTOR_CHECKPOINT();

    BOOST_ACTOR_PRINT("test sync send");

    BOOST_ACTOR_CHECKPOINT();
    auto sync_testee = spawn<blocking_api>([](blocking_actor* s) {
        s->receive (
            on("hi", arg_match) >> [&](actor from) {
                s->sync_send(from, "whassup?", s).await(
                    on_arg_match >> [&](const string& str) -> string {
                        BOOST_ACTOR_CHECK(s->last_sender() != nullptr);
                        BOOST_ACTOR_CHECK_EQUAL(str, "nothing");
                        return "goodbye!";
                    },
                    after(chrono::minutes(1)) >> [] {
                        cerr << "PANIC!!!!" << endl;
                        abort();
                    }
                );
            },
            others() >> BOOST_ACTOR_UNEXPECTED_MSG_CB_REF(s)
        );
    });
    self->monitor(sync_testee);
    self->send(sync_testee, "hi", self);
    self->receive (
        on("whassup?", arg_match) >> [&](actor other) -> std::string {
            BOOST_ACTOR_CHECKPOINT();
            // this is NOT a reply, it's just an asynchronous message
            self->send(other, "a lot!");
            return "nothing";
        }
    );
    self->receive (
        on("goodbye!") >> BOOST_ACTOR_CHECKPOINT_CB(),
        after(std::chrono::seconds(5)) >> BOOST_ACTOR_UNEXPECTED_TOUT_CB()
    );
    self->receive (
        on_arg_match >> [&](const down_msg& dm) {
            BOOST_ACTOR_CHECK_EQUAL(dm.reason, exit_reason::normal);
            BOOST_ACTOR_CHECK_EQUAL(dm.source, sync_testee);
        }
    );
    self->await_all_other_actors_done();
    BOOST_ACTOR_CHECKPOINT();

    self->sync_send(sync_testee, "!?").await(
        on<sync_exited_msg>() >> BOOST_ACTOR_CHECKPOINT_CB(),
        others() >> BOOST_ACTOR_UNEXPECTED_MSG_CB_REF(self),
        after(chrono::milliseconds(5)) >> BOOST_ACTOR_UNEXPECTED_TOUT_CB()
    );

    BOOST_ACTOR_CHECKPOINT();

    auto inflater = [](event_based_actor* s, const string& name, actor buddy) {
        BOOST_ACTOR_LOGF_TRACE(BOOST_ACTOR_ARG(s) << ", " << BOOST_ACTOR_ARG(name)
                        << ", " << BOOST_ACTOR_TARG(buddy, to_string));
        s->become(
            on_arg_match >> [=](int n, const string& str) {
                s->send(buddy, n * 2, str + " from " + name);
            },
            on(atom("done")) >> [=] {
                s->quit();
            }
        );
    };
    auto joe = spawn(inflater, "Joe", self);
    auto bob = spawn(inflater, "Bob", joe);
    self->send(bob, 1, "hello actor");
    self->receive (
        on(4, "hello actor from Bob from Joe") >> BOOST_ACTOR_CHECKPOINT_CB(),
        others() >> BOOST_ACTOR_UNEXPECTED_MSG_CB_REF(self)
    );
    // kill joe and bob
    auto poison_pill = make_any_tuple(atom("done"));
    anon_send_tuple(joe, poison_pill);
    anon_send_tuple(bob, poison_pill);
    self->await_all_other_actors_done();

    function<actor (const string&, const actor&)> spawn_next;
    // it's safe to capture spawn_next as reference here, because
    // - it is guaranteeed to outlive kr34t0r by general scoping rules
    // - the lambda is always executed in the current actor's thread
    // but using spawn_next in a message handler could
    // still cause undefined behavior!
    auto kr34t0r = [&spawn_next](event_based_actor* s, const string& name, actor pal) {
        if (name == "Joe" && !pal) {
            pal = spawn_next("Bob", s);
        }
        s->become (
            others() >> [=] {
                // forward message and die
                s->send_tuple(pal, s->last_dequeued());
                s->quit();
            }
        );
    };
    spawn_next = [&kr34t0r](const string& name, const actor& pal) {
        return spawn(kr34t0r, name, pal);
    };
    auto joe_the_second = spawn(kr34t0r, "Joe", invalid_actor);
    self->send(joe_the_second, atom("done"));
    self->await_all_other_actors_done();

    auto f = [](const string& name) -> behavior {
        return (
            on(atom("get_name")) >> [name] {
                return std::make_tuple(atom("name"), name);
            }
        );
    };
    auto a1 = spawn(f, "alice");
    auto a2 = spawn(f, "bob");
    self->send(a1, atom("get_name"));
    self->receive (
        on(atom("name"), arg_match) >> [&](const string& name) {
            BOOST_ACTOR_CHECK_EQUAL(name, "alice");
        }
    );
    self->send(a2, atom("get_name"));
    self->receive (
        on(atom("name"), arg_match) >> [&](const string& name) {
            BOOST_ACTOR_CHECK_EQUAL(name, "bob");
        }
    );
    self->send_exit(a1, exit_reason::user_shutdown);
    self->send_exit(a2, exit_reason::user_shutdown);
    self->await_all_other_actors_done();
    BOOST_ACTOR_CHECKPOINT();

    auto res1 = behavior_test<testee_actor>(self, spawn<blocking_api>(testee_actor{}));
    BOOST_ACTOR_CHECK_EQUAL("wait4int", res1);
    BOOST_ACTOR_CHECK_EQUAL(behavior_test<event_testee>(self, spawn<event_testee>()), "wait4int");
    self->await_all_other_actors_done();
    BOOST_ACTOR_CHECKPOINT();

    // create some actors linked to one single actor
    // and kill them all through killing the link
    auto legion = spawn([](event_based_actor* s) {
        BOOST_ACTOR_PRINT("spawn 100 actors");
        for (int i = 0; i < 100; ++i) {
            s->spawn<event_testee, linked>();
        }
        s->become(others() >> BOOST_ACTOR_UNEXPECTED_MSG_CB(s));
    });
    self->send_exit(legion, exit_reason::user_shutdown);
    self->await_all_other_actors_done();
    BOOST_ACTOR_CHECKPOINT();
    self->trap_exit(true);
    auto ping_actor = self->spawn<monitored+blocking_api>(ping, 10);
    auto pong_actor = self->spawn<monitored+blocking_api>(pong, ping_actor);
    self->link_to(pong_actor);
    int i = 0;
    int flags = 0;
    self->delayed_send(self, chrono::seconds(1), atom("FooBar"));
    // wait for DOWN and EXIT messages of pong
    self->receive_for(i, 4) (
        on_arg_match >> [&](const exit_msg& em) {
            BOOST_ACTOR_CHECK_EQUAL(em.source, pong_actor);
            BOOST_ACTOR_CHECK_EQUAL(em.reason, exit_reason::user_shutdown);
            flags |= 0x01;
        },
        on_arg_match >> [&](const down_msg& dm) {
            if (dm.source == pong_actor) {
                flags |= 0x02;
                BOOST_ACTOR_CHECK_EQUAL(dm.reason, exit_reason::user_shutdown);
            }
            else if (dm.source == ping_actor) {
                flags |= 0x04;
                BOOST_ACTOR_CHECK_EQUAL(dm.reason, exit_reason::normal);
            }
        },
        on_arg_match >> [&](const atom_value& val) {
            BOOST_ACTOR_CHECK(val == atom("FooBar"));
            flags |= 0x08;
        },
        others() >> [&]() {
            BOOST_ACTOR_FAILURE("unexpected message: " << to_string(self->last_dequeued()));
        },
        after(chrono::seconds(5)) >> [&]() {
            BOOST_ACTOR_FAILURE("timeout in file " << __FILE__ << " in line " << __LINE__);
        }
    );
    // wait for termination of all spawned actors
    self->await_all_other_actors_done();
    BOOST_ACTOR_CHECK_EQUAL(flags, 0x0F);
    // verify pong messages
    BOOST_ACTOR_CHECK_EQUAL(pongs(), 10);
    BOOST_ACTOR_CHECKPOINT();
    spawn<priority_aware>(high_priority_testee);
    self->await_all_other_actors_done();
    BOOST_ACTOR_CHECKPOINT();
    spawn<high_priority_testee_class, priority_aware>();
    self->await_all_other_actors_done();
    // test sending message to self via scoped_actor
    self->send(self, atom("check"));
    self->receive (
        on(atom("check")) >> [] {
            BOOST_ACTOR_CHECKPOINT();
        }
    );
    BOOST_ACTOR_CHECKPOINT();
    BOOST_ACTOR_PRINT("check whether timeouts trigger more than once");
    auto counter = make_shared<int>(0);
    auto sleeper = self->spawn<monitored>([=](event_based_actor* s) {
        return after(std::chrono::milliseconds(1)) >> [=] {
            BOOST_ACTOR_PRINT("received timeout #" << (*counter + 1));
            if (++*counter > 3) {
                BOOST_ACTOR_CHECKPOINT();
                s->quit();
            }
        };
    });
    self->receive(
        [&](const down_msg& msg) {
            BOOST_ACTOR_CHECK_EQUAL(msg.source, sleeper);
            BOOST_ACTOR_CHECK_EQUAL(msg.reason, exit_reason::normal);
        }
    );
    BOOST_ACTOR_CHECKPOINT();
}

} // namespace <anonymous>

int main() {
    BOOST_ACTOR_TEST(test_spawn);
    test_spawn();
    BOOST_ACTOR_CHECKPOINT();
    shutdown();
    BOOST_ACTOR_CHECKPOINT();
    return BOOST_ACTOR_TEST_RESULT();
}
