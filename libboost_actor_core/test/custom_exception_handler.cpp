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

#define BOOST_TEST_MODULE custome_exception_handler
#include <boost/test/included/unit_test.hpp>

#include "boost/actor/all.hpp"

using boost::none;
using boost::join;
using boost::variant;
using boost::optional;
using boost::is_any_of;
using boost::token_compress_on;
using namespace boost::actor;

class exception_testee : public event_based_actor {
public:
  ~exception_testee();
  exception_testee() {
    set_exception_handler([](const std::exception_ptr&) -> optional<uint32_t> {
      return exit_reason::user_defined + 2;
    });
  }
  behavior make_behavior() override {
    return {
      others >> [] {
        throw std::runtime_error("whatever");
      }
    };
  }
};

exception_testee::~exception_testee() {
  // avoid weak-vtables warning
}


BOOST_AUTO_TEST_CASE(test_custom_exception_handler) {
  auto handler = [](const std::exception_ptr& eptr) -> optional<uint32_t> {
    try {
      std::rethrow_exception(eptr);
    }
    catch (std::runtime_error&) {
      return exit_reason::user_defined;
    }
    catch (...) {
      // "fall through"
    }
    return exit_reason::user_defined + 1;
  };
  {
    scoped_actor self;
    auto testee1 = self->spawn<monitored>([=](event_based_actor* eb_self) {
      eb_self->set_exception_handler(handler);
      throw std::runtime_error("ping");
    });
    auto testee2 = self->spawn<monitored>([=](event_based_actor* eb_self) {
      eb_self->set_exception_handler(handler);
      throw std::logic_error("pong");
    });
    auto testee3 = self->spawn<exception_testee, monitored>();
    self->send(testee3, "foo");
    // receive all down messages
    auto i = 0;
    self->receive_for(i, 3)(
      [&](const down_msg& dm) {
        if (dm.source == testee1) {
          BOOST_CHECK_EQUAL(dm.reason, exit_reason::user_defined);
        }
        else if (dm.source == testee2) {
          BOOST_CHECK_EQUAL(dm.reason, exit_reason::user_defined + 1);
        }
        else if (dm.source == testee3) {
          BOOST_CHECK_EQUAL(dm.reason, exit_reason::user_defined + 2);
        }
        else {
          BOOST_CHECK(false); // report error
        }
      }
    );
    self->await_all_other_actors_done();
  }
  shutdown();
}
