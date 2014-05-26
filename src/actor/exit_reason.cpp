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
 * Dominik Charousset <dominik.charousset (at) haw-hamburg.de>                *
 *                                                                            *
 * Distributed under the Boost Software License, Version 1.0. See             *
 * accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt  *
\******************************************************************************/


#include "boost/actor/exit_reason.hpp"

namespace boost {
namespace actor { namespace exit_reason {

namespace { constexpr const char* s_names_table[] = {
    "not_exited",
    "normal",
    "unhandled_exception",
    "unallowed_function_call",
    "unhandled_sync_failure",
    "unhandled_sync_timeout"
}; }

const char* as_string(std::uint32_t value) {
    if (value <= unhandled_sync_timeout) return s_names_table[value];
    if (value == remote_link_unreachable) return "remote_link_unreachable";
    if (value >= user_defined) return "user_defined";
    return "illegal_exit_reason";
}

} } // namespace actor
} // namespace boost::exit_reason
