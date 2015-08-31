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

#ifndef BOOST_ACTOR_SET_SCHEDULER_HPP
#define BOOST_ACTOR_SET_SCHEDULER_HPP

#include "boost/actor/config.hpp"

#include <thread>
#include <limits>

#include "boost/actor/policy/work_stealing.hpp"

#include "boost/actor/scheduler/worker.hpp"
#include "boost/actor/scheduler/coordinator.hpp"
#include "boost/actor/scheduler/abstract_coordinator.hpp"

namespace boost {
namespace actor {
/// Sets a user-defined scheduler.
/// @note This function must be used before actor is spawned. Dynamically
///       changing the scheduler at runtime is not supported.
/// @throws std::logic_error if a scheduler is already defined
void set_scheduler(scheduler::abstract_coordinator* ptr);

/// Sets a user-defined scheduler using given policies. The scheduler
/// is instantiated with `nw` number of workers and allows each actor
/// to consume up to `max_throughput` per resume (must be > 0).
/// @note This function must be used before actor is spawned. Dynamically
///       changing the scheduler at runtime is not supported.
/// @throws std::logic_error if a scheduler is already defined
/// @throws std::invalid_argument if `max_throughput == 0`
template <class Policy = policy::work_stealing>
void set_scheduler(size_t nw = std::thread::hardware_concurrency(),
                   size_t max_throughput = std::numeric_limits<size_t>::max()) {
  if (max_throughput == 0) {
    throw std::invalid_argument("max_throughput must not be 0");
  }
  set_scheduler(new scheduler::coordinator<Policy>(nw, max_throughput));
}

} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_SET_SCHEDULER_HPP
