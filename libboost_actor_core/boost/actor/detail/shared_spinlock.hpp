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

#ifndef BOOST_ACTOR_DETAIL_SHARED_SPINLOCK_HPP
#define BOOST_ACTOR_DETAIL_SHARED_SPINLOCK_HPP

#include <atomic>
#include <cstddef>

namespace boost {
namespace actor {
namespace detail {

/// A spinlock implementation providing shared and exclusive locking.
class shared_spinlock {

  std::atomic<long> flag_;

public:

  shared_spinlock();

  void lock();
  void unlock();
  bool try_lock();

  void lock_shared();
  void unlock_shared();
  bool try_lock_shared();

  void lock_upgrade();
  void unlock_upgrade();
  void unlock_upgrade_and_lock();
  void unlock_and_lock_upgrade();

};

} // namespace detail
} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_DETAIL_SHARED_SPINLOCK_HPP
