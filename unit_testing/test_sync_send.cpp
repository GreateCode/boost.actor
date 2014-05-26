#include "test.hpp"

#include "boost/none.hpp"

#include "boost/actor/all.hpp"

using boost::none;
using boost::optional;

using namespace std;
using namespace boost::actor;

namespace {

struct sync_mirror : sb_actor<sync_mirror> {
    behavior init_state;

    sync_mirror() {
        init_state = (
            others() >> [=] { return last_dequeued(); }
        );
    }
};

// replies to 'f' with 0.0f and to 'i' with 0
struct float_or_int : sb_actor<float_or_int> {
    behavior init_state = (
        on(atom("f")) >> [] { return 0.0f; },
        on(atom("i")) >> [] { return 0; }
    );
};

struct popular_actor : event_based_actor { // popular actors have a buddy
    actor m_buddy;
    popular_actor(const actor& buddy) : m_buddy(buddy) { }
    inline const actor& buddy() const { return m_buddy; }
    void report_failure() {
        send(buddy(), atom("failure"));
        quit();
    }
};


/******************************************************************************\
 *                                test case 1:                                *
 *                                                                            *
 *                  A                  B                  C                   *
 *                  |                  |                  |                   *
 *                  | --(sync_send)--> |                  |                   *
 *                  |                  | --(forward)----> |                   *
 *                  |                  X                  |---\               *
 *                  |                                     |   |               *
 *                  |                                     |<--/               *
 *                  | <-------------(reply)-------------- |                   *
 *                  X                                     X                   *
\******************************************************************************/

struct A : popular_actor {
    A(const actor& buddy) : popular_actor(buddy) { }
    behavior make_behavior() override {
        return (
            on(atom("go"), arg_match) >> [=](const actor& next) {
                BOOST_ACTOR_CHECKPOINT();
                sync_send(next, atom("gogo")).then([=] {
                    BOOST_ACTOR_CHECKPOINT();
                    send(buddy(), atom("success"));
                    quit();
                });
            },
            others() >> [=] { report_failure(); }
        );
    }
};

struct B : popular_actor {
    B(const actor& buddy) : popular_actor(buddy) { }
    behavior make_behavior() override {
        return (
            others() >> [=] {
                BOOST_ACTOR_CHECKPOINT();
                forward_to(buddy());
                quit();
            }
        );
    }
};

struct C : sb_actor<C> {
    behavior init_state;
    C() {
        init_state = (
            on(atom("gogo")) >> [=]() -> atom_value {
                BOOST_ACTOR_CHECKPOINT();
                quit();
                return atom("gogogo");
            }
        );
    }
};


/******************************************************************************\
 *                                test case 2:                                *
 *                                                                            *
 *                  A                  D                  C                   *
 *                  |                  |                  |                   *
 *                  | --(sync_send)--> |                  |                   *
 *                  |                  | --(sync_send)--> |                   *
 *                  |                  |                  |---\               *
 *                  |                  |                  |   |               *
 *                  |                  |                  |<--/               *
 *                  |                  | <---(reply)----- |                   *
 *                  | <---(reply)----- |                                      *
 *                  X                  X                                      *
\******************************************************************************/

struct D : popular_actor {
    D(const actor& buddy) : popular_actor(buddy) { }
    behavior make_behavior() override {
        return (
            others() >> [=] {
                /*
                response_promise handle = make_response_promise();
                sync_send_tuple(buddy(), last_dequeued()).then([=] {
                    reply_to(handle, last_dequeued());
                    quit();
                });
                //*/
                return sync_send_tuple(buddy(), last_dequeued()).then([=]() -> message {
                    quit();
                    return last_dequeued();
                });//*/
            }
        );
    }
};


/******************************************************************************\
 *                                test case 3:                                *
 *                                                                            *
 *                Client             Server             Worker                *
 *                  |                  |                  |                   *
 *                  |                  | <---(idle)------ |                   *
 *                  | ---(request)---> |                  |                   *
 *                  |                  | ---(request)---> |                   *
 *                  |                  |                  |---\               *
 *                  |                  X                  |   |               *
 *                  |                                     |<--/               *
 *                  | <------------(response)------------ |                   *
 *                  X                                                         *
\******************************************************************************/

struct server : event_based_actor {

    behavior make_behavior() override {
        auto die = [=] { quit(exit_reason::user_shutdown); };
        return (
            on(atom("idle"), arg_match) >> [=](actor worker) {
                become (
                    keep_behavior,
                    on(atom("request")) >> [=] {
                        forward_to(worker);
                        unbecome(); // await next idle message
                    },
                    on(atom("idle")) >> skip_message,
                    others() >> die
                );
            },
            on(atom("request")) >> skip_message,
            others() >> die
        );
    }

};

void compile_time_optional_variant_check(event_based_actor* self) {
    typedef boost::variant<std::tuple<int, float>,
                           std::tuple<float, int, int>>
            msg_type;
    self->sync_send(self, atom("msg")).then([](msg_type) {});
}

void test_sync_send() {
    scoped_actor self;
    self->on_sync_failure([&] {
        BOOST_ACTOR_FAILURE("received: " << to_string(self->last_dequeued()));
    });
    self->spawn<monitored + blocking_api>([](blocking_actor* s) {
        BOOST_ACTOR_LOGC_TRACE("NONE", "main$sync_failure_test", "id = " << s->id());
        int invocations = 0;
        auto foi = s->spawn<float_or_int, linked>();
        s->send(foi, atom("i"));
        s->receive(on_arg_match >> [](int i) { BOOST_ACTOR_CHECK_EQUAL(i, 0); });
        s->on_sync_failure([=] {
            BOOST_ACTOR_FAILURE("received: " << to_string(s->last_dequeued()));
        });
        s->sync_send(foi, atom("i")).await(
            [&](int i) { BOOST_ACTOR_CHECK_EQUAL(i, 0); ++invocations; },
            [&](float) { BOOST_ACTOR_UNEXPECTED_MSG(s); }
        );
        s->sync_send(foi, atom("f")).await(
            [&](int) { BOOST_ACTOR_UNEXPECTED_MSG(s); },
            [&](float f) { BOOST_ACTOR_CHECK_EQUAL(f, 0.f); ++invocations; }
        );
        BOOST_ACTOR_CHECK_EQUAL(invocations, 2);
        BOOST_ACTOR_PRINT("trigger sync failure");
        // provoke invocation of s->handle_sync_failure()
        bool sync_failure_called = false;
        bool int_handler_called = false;
        s->on_sync_failure([&] {
            sync_failure_called = true;
        });
        s->sync_send(foi, atom("f")).await(
            on<int>() >> [&] {
                int_handler_called = true;
            }
        );
        BOOST_ACTOR_CHECK_EQUAL(sync_failure_called, true);
        BOOST_ACTOR_CHECK_EQUAL(int_handler_called, false);
        s->quit(exit_reason::user_shutdown);
    });
    self->receive (
        on_arg_match >> [&](const down_msg& dm) {
            BOOST_ACTOR_CHECK_EQUAL(dm.reason, exit_reason::user_shutdown);
        },
        others() >> BOOST_ACTOR_UNEXPECTED_MSG_CB_REF(self)
    );
    auto mirror = spawn<sync_mirror>();
    bool continuation_called = false;
    self->sync_send(mirror, 42)
    .await([&](int value) {
        continuation_called = true;
        BOOST_ACTOR_CHECK_EQUAL(value, 42);
    });
    BOOST_ACTOR_CHECK_EQUAL(continuation_called, true);
    self->send_exit(mirror, exit_reason::user_shutdown);
    self->await_all_other_actors_done();
    BOOST_ACTOR_CHECKPOINT();
    auto non_normal_down_msg = [](down_msg dm) -> optional<down_msg> {
        if (dm.reason != exit_reason::normal) return dm;
        return none;
    };
    auto await_success_message = [&] {
        self->receive (
            on(atom("success")) >> BOOST_ACTOR_CHECKPOINT_CB(),
            on(atom("failure")) >> BOOST_ACTOR_FAILURE_CB("A didn't receive sync response"),
            on(non_normal_down_msg) >> [&](const down_msg& dm) {
                BOOST_ACTOR_FAILURE("A exited for reason " << dm.reason);
            }
        );
    };
    self->send(self->spawn<A, monitored>(self), atom("go"), spawn<B>(spawn<C>()));
    await_success_message();
    BOOST_ACTOR_CHECKPOINT();
    self->await_all_other_actors_done();
    self->send(self->spawn<A, monitored>(self), atom("go"), spawn<D>(spawn<C>()));
    await_success_message();
    BOOST_ACTOR_CHECKPOINT();
    self->await_all_other_actors_done();
    BOOST_ACTOR_CHECKPOINT();
    self->timed_sync_send(self, std::chrono::milliseconds(50), atom("NoWay")).await(
        on<sync_timeout_msg>() >> BOOST_ACTOR_CHECKPOINT_CB(),
        others() >> BOOST_ACTOR_UNEXPECTED_MSG_CB_REF(self)
    );
    // we should have received two DOWN messages with normal exit reason
    // plus 'NoWay'
    int i = 0;
    self->receive_for(i, 3) (
        on_arg_match >> [&](const down_msg& dm) {
            BOOST_ACTOR_CHECK_EQUAL(dm.reason, exit_reason::normal);
        },
        on(atom("NoWay")) >> [] {
            BOOST_ACTOR_CHECKPOINT();
            BOOST_ACTOR_PRINT("trigger \"actor did not reply to a "
                       "synchronous request message\"");
        },
        others() >> BOOST_ACTOR_UNEXPECTED_MSG_CB_REF(self),
        after(std::chrono::seconds(0)) >> BOOST_ACTOR_UNEXPECTED_TOUT_CB()
    );
    BOOST_ACTOR_CHECKPOINT();
    // mailbox should be empty now
    self->receive (
        others() >> BOOST_ACTOR_UNEXPECTED_MSG_CB_REF(self),
        after(std::chrono::seconds(0)) >> BOOST_ACTOR_CHECKPOINT_CB()
    );
    // check wheter continuations are invoked correctly
    auto c = spawn<C>(); // replies only to 'gogo' messages
    // first test: sync error must occur, continuation must not be called
    bool timeout_occured = false;
    self->on_sync_timeout([&] { timeout_occured = true; });
    self->on_sync_failure(BOOST_ACTOR_UNEXPECTED_MSG_CB_REF(self));
    self->timed_sync_send(c, std::chrono::milliseconds(500), atom("HiThere"))
    .await(BOOST_ACTOR_FAILURE_CB("C replied to 'HiThere'!"));
    BOOST_ACTOR_CHECK_EQUAL(timeout_occured, true);
    self->on_sync_failure(BOOST_ACTOR_UNEXPECTED_MSG_CB_REF(self));
    self->sync_send(c, atom("gogo")).await(BOOST_ACTOR_CHECKPOINT_CB());
    self->send_exit(c, exit_reason::user_shutdown);
    self->await_all_other_actors_done();
    BOOST_ACTOR_CHECKPOINT();

    // test use case 3
    self->spawn<monitored + blocking_api>([](blocking_actor* s) { // client
        auto serv = s->spawn<server, linked>();                   // server
        auto work = s->spawn<linked>([](event_based_actor* w) {   // worker
            w->become(on(atom("request")) >> []{ return atom("response"); });
        });
        // first 'idle', then 'request'
        anon_send(serv, atom("idle"), work);
        s->sync_send(serv, atom("request")).await(
            on(atom("response")) >> [=] {
                BOOST_ACTOR_CHECKPOINT();
                BOOST_ACTOR_CHECK_EQUAL(s->last_sender(), work);
            },
            others() >> [&] {
                BOOST_ACTOR_PRINTERR("unexpected message: "
                              << to_string(s->last_dequeued()));
            }
        );
        // first 'request', then 'idle'
        auto handle = s->sync_send(serv, atom("request"));
        send_as(work, serv, atom("idle"));
        handle.await(
            on(atom("response")) >> [=] {
                BOOST_ACTOR_CHECKPOINT();
                BOOST_ACTOR_CHECK_EQUAL(s->last_sender(), work);
            },
            others() >> BOOST_ACTOR_UNEXPECTED_MSG_CB(s)
        );
        s->send(s, "Ever danced with the devil in the pale moonlight?");
        // response: {'EXIT', exit_reason::user_shutdown}
        s->receive_loop(others() >> BOOST_ACTOR_UNEXPECTED_MSG_CB(s));
    });
    self->receive (
        on_arg_match >> [&](const down_msg& dm) {
            BOOST_ACTOR_CHECK_EQUAL(dm.reason, exit_reason::user_shutdown);
        },
        others() >> BOOST_ACTOR_UNEXPECTED_MSG_CB_REF(self)
    );
}

} // namespace <anonymous>

int main() {
    BOOST_ACTOR_TEST(test_sync_send);
    test_sync_send();
    await_all_actors_done();
    BOOST_ACTOR_CHECKPOINT();
    // shutdown warning about unused function (only performs compile-time check)
    static_cast<void>(compile_time_optional_variant_check);
    return BOOST_ACTOR_TEST_RESULT();
}
