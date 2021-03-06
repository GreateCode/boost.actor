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

#include "boost/actor/scheduler/detached_threads.hpp"

#include <mutex>
#include <atomic>
#include <condition_variable>

namespace boost {
namespace actor {
namespace scheduler {

namespace {

std::atomic<size_t> s_detached;
std::mutex s_detached_mtx;
std::condition_variable s_detached_cv;

} // namespace <anonymous>


void inc_detached_threads() {
  ++s_detached;
}

void dec_detached_threads() {
  size_t new_val = --s_detached;
  if (new_val == 0) {
    std::unique_lock<std::mutex> guard(s_detached_mtx);
    s_detached_cv.notify_all();
  }
}

void await_detached_threads() {
  std::unique_lock<std::mutex> guard{s_detached_mtx};
  while (s_detached != 0) {
    s_detached_cv.wait(guard);
  }
}

} // namespace scheduler
} // namespace actor
} // namespace boost
