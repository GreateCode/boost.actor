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
 * Copyright (C) 2011 - 2014                                                  *
 * Dominik Charousset <dominik.charousset@haw-hamburg.de>                     *
 *                                                                            *
 * Distributed under the Boost Software License, Version 1.0. See             *
 * accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt  *
\******************************************************************************/


#include <memory>
#include <iostream>

#include "test.hpp"
#include "boost/actor/all.hpp"

using namespace std;
using namespace boost::actor;

void ping(event_based_actor* self, size_t num_pings) {
    BOOST_ACTOR_CHECKPOINT();
    auto count = std::make_shared<size_t>(0);
    self->become (
        on(atom("kickoff"), arg_match) >> [=](const actor& pong) {
            BOOST_ACTOR_CHECKPOINT();
            self->send(pong, atom("ping"), 1);
            self->become (
                on(atom("pong"), arg_match)
                >> [=](int value) -> std::tuple<atom_value, int> {
                    if (++*count >= num_pings) self->quit();
                    return std::make_tuple(atom("ping"), value + 1);
                },
                others() >> BOOST_ACTOR_UNEXPECTED_MSG_CB(self)
            );
        },
        others() >> BOOST_ACTOR_UNEXPECTED_MSG_CB(self)
    );
}

void pong(event_based_actor* self) {
    BOOST_ACTOR_CHECKPOINT();
    self->become  (
        on(atom("ping"), arg_match)
        >> [=](int value) -> std::tuple<atom_value, int> {
            BOOST_ACTOR_CHECKPOINT();
            self->monitor(self->last_sender());
            // set next behavior
            self->become (
                on(atom("ping"), arg_match) >> [](int val) {
                    return std::make_tuple(atom("pong"), val);
                },
                on_arg_match >> [=](const down_msg& dm) {
                    self->quit(dm.reason);
                },
                others() >> BOOST_ACTOR_UNEXPECTED_MSG_CB(self)
            );
            // reply to 'ping'
            return {atom("pong"), value};
        },
        others() >> BOOST_ACTOR_UNEXPECTED_MSG_CB(self)
    );
}

void peer(io::broker* self, io::connection_handle hdl, const actor& buddy) {
    BOOST_ACTOR_CHECKPOINT();
    BOOST_ACTOR_CHECK(self != nullptr);
    BOOST_ACTOR_CHECK(buddy != invalid_actor);
    self->monitor(buddy);
    if (self->num_connections() == 0) {
        cerr << "num_connections() != 1" << endl;
        throw std::logic_error("num_connections() != 1");
    }
    auto write = [=](atom_value type, int value) {
        BOOST_ACTOR_LOGF_DEBUG("write: " << value);
        self->write(hdl, sizeof(type), &type);
        self->write(hdl, sizeof(value), &value);
    };
    self->become (
        [=](const connection_closed_msg&) {
            BOOST_ACTOR_PRINT("received connection_closed_msg");
            self->quit();
        },
        [=](const new_data_msg& msg) {
            atom_value type;
            int value;
            memcpy(&type, msg.buf.data(), sizeof(atom_value));
            memcpy(&value, msg.buf.offset_data(sizeof(atom_value)), sizeof(int));
            self->send(buddy, type, value);
        },
        on(atom("ping"), arg_match) >> [=](int value) {
            write(atom("ping"), value);
        },
        on(atom("pong"), arg_match) >> [=](int value) {
            write(atom("pong"), value);
        },
        [=](const down_msg& dm) {
            if (dm.source == buddy) self->quit(dm.reason);
        },
        others() >> BOOST_ACTOR_UNEXPECTED_MSG_CB(self)
    );
}

void peer_acceptor(io::broker* self, const actor& buddy) {
    BOOST_ACTOR_CHECKPOINT();
    self->become (
        [=](const new_connection_msg& msg) {
            BOOST_ACTOR_CHECKPOINT();
            BOOST_ACTOR_PRINT("received new_connection_msg");
            self->fork(peer, msg.handle, buddy);
            self->quit();
        },
        others() >> BOOST_ACTOR_UNEXPECTED_MSG_CB(self)
    );
}

int main(int argc, char** argv) {
    BOOST_ACTOR_TEST(test_broker);
    string app_path = argv[0];
    if (argc == 3) {
        if (strcmp(argv[1], "mode=client") == 0) {
            BOOST_ACTOR_CHECKPOINT();
            run_client_part(get_kv_pairs(argc, argv), [](uint16_t port) {
                BOOST_ACTOR_CHECKPOINT();
                auto p = spawn(ping, 10);
                BOOST_ACTOR_CHECKPOINT();
                auto cl = spawn_io_client(peer, "localhost", port, p);
                BOOST_ACTOR_CHECKPOINT();
                anon_send(p, atom("kickoff"), cl);
                BOOST_ACTOR_CHECKPOINT();
            });
            BOOST_ACTOR_CHECKPOINT();
            return BOOST_ACTOR_TEST_RESULT();
        }
        return BOOST_ACTOR_TEST_RESULT();
    }
    else if (argc > 1) {
        cerr << "usage: " << app_path << " [mode=client port={PORT}]" << endl;
        return -1;
    }
    BOOST_ACTOR_CHECKPOINT();
    auto p = spawn(pong);
    uint16_t port = 4242;
    for (;;) {
        try {
            spawn_io_server(peer_acceptor, port, p);
            BOOST_ACTOR_CHECKPOINT();
            ostringstream oss;
            oss << app_path << " mode=client port=" << port << to_dev_null;
            thread child{[&oss] {
                BOOST_ACTOR_LOGC_TRACE("NONE", "main$thread_launcher", "");
                auto cmdstr = oss.str();
                if (system(cmdstr.c_str()) != 0) {
                    BOOST_ACTOR_PRINTERR("FATAL: command failed: " << cmdstr);
                    abort();
                }
            }};
            BOOST_ACTOR_CHECKPOINT();
            child.join();
            BOOST_ACTOR_CHECKPOINT();
            await_all_actors_done();
            BOOST_ACTOR_CHECKPOINT();
            shutdown();
            return BOOST_ACTOR_TEST_RESULT();
        }
        catch (bind_failure&) {
            // try next port
            ++port;
        }
    }
}

