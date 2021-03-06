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

#ifndef BOOST_ACTOR_DETAIL_RUN_PROGRAM_HPP
#define BOOST_ACTOR_DETAIL_RUN_PROGRAM_HPP

#include <thread>
#include <vector>
#include <string>
#include <initializer_list>

#include "boost/actor/send.hpp"
#include "boost/actor/actor.hpp"
#include "boost/actor/message.hpp"

namespace boost {
namespace actor {
namespace detail {

std::thread run_sub_unit_test(boost::actor::actor listener,
                              const char* path,
                              int unsued_int,
                              const char* test_module_name,
                              bool unused_bool,
                              std::initializer_list<std::string> args);

} // namespace detail
} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_DETAIL_RUN_PROGRAM_HPP
