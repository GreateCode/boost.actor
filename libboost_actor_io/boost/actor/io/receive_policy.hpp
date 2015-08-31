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

#ifndef BOOST_ACTOR_IO_RECEIVE_POLICY_HPP
#define BOOST_ACTOR_IO_RECEIVE_POLICY_HPP

#include <cstddef>
#include <utility>

#include "boost/actor/config.hpp"

namespace boost {
namespace actor {
namespace io {

enum class receive_policy_flag {
  at_least,
  at_most,
  exactly
};

class receive_policy {

  receive_policy() = delete;

public:

  using config = std::pair<receive_policy_flag, size_t>;

  static inline config at_least(size_t num_bytes) {
    BOOST_ACTOR_ASSERT(num_bytes > 0);
    return {receive_policy_flag::at_least, num_bytes};
  }

  static inline config at_most(size_t num_bytes) {
    BOOST_ACTOR_ASSERT(num_bytes > 0);
    return {receive_policy_flag::at_most, num_bytes};
  }

  static inline config exactly(size_t num_bytes) {
    BOOST_ACTOR_ASSERT(num_bytes > 0);
    return {receive_policy_flag::exactly, num_bytes};
  }

};

} // namespace io
} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_IO_RECEIVE_POLICY_HPP
