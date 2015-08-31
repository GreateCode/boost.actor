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

#include <limits>
#include <thread>

#include "boost/actor/detail/shared_spinlock.hpp"

#include "boost/actor/detail/cas_weak.hpp"

namespace {

inline long min_long() {
  return std::numeric_limits<long>::min();
}

} // namespace <anonymous>

namespace boost {
namespace actor {
namespace detail {

shared_spinlock::shared_spinlock() : flag_(0) {
  // nop
}

void shared_spinlock::lock() {
  long v = flag_.load();
  for (;;) {
    if (v != 0) {
      v = flag_.load();
    } else if (cas_weak(&flag_, &v, min_long())) {
      return;
    }
    // else: next iteration
  }
}

void shared_spinlock::lock_upgrade() {
  lock_shared();
}

void shared_spinlock::unlock_upgrade() {
  unlock_shared();
}

void shared_spinlock::unlock_upgrade_and_lock() {
  unlock_shared();
  lock();
}

void shared_spinlock::unlock_and_lock_upgrade() {
  unlock();
  lock_upgrade();
}

void shared_spinlock::unlock() {
  flag_.store(0);
}

bool shared_spinlock::try_lock() {
  long v = flag_.load();
  return (v == 0) ? cas_weak(&flag_, &v, min_long()) : false;
}

void shared_spinlock::lock_shared() {
  long v = flag_.load();
  for (;;) {
    if (v < 0) {
      // std::this_thread::yield();
      v = flag_.load();
    } else if (cas_weak(&flag_, &v, v + 1)) {
      return;
    }
    // else: next iteration
  }
}

void shared_spinlock::unlock_shared() {
  flag_.fetch_sub(1);
}

bool shared_spinlock::try_lock_shared() {
  long v = flag_.load();
  return (v >= 0) ? cas_weak(&flag_, &v, v + 1) : false;
}

} // namespace detail
} // namespace actor
} // namespace boost
