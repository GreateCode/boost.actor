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

#ifndef BOOST_ACTOR_SCHEDULER_ABSTRACT_COORDINATOR_HPP
#define BOOST_ACTOR_SCHEDULER_ABSTRACT_COORDINATOR_HPP

#include <chrono>
#include <atomic>
#include <cstddef>

#include "boost/actor/fwd.hpp"
#include "boost/actor/atom.hpp"
#include "boost/actor/actor.hpp"
#include "boost/actor/channel.hpp"
#include "boost/actor/message.hpp"
#include "boost/actor/duration.hpp"
#include "boost/actor/actor_addr.hpp"

namespace boost {
namespace actor {
namespace scheduler {

/// A coordinator creates the workers, manages delayed sends and
/// the central printer instance for {@link aout}. It also forwards
/// sends from detached workers or non-actor threads to randomly
/// chosen workers.
class abstract_coordinator {
public:
  friend class detail::singletons;

  explicit abstract_coordinator(size_t num_worker_threads);

  virtual ~abstract_coordinator();

  /// Returns a handle to the central printing actor.
  inline actor printer() const {
    return printer_;
  }

  /// Puts `what` into the queue of a randomly chosen worker.
  virtual void enqueue(resumable* what) = 0;

  template <class Duration, class... Data>
  void delayed_send(Duration rel_time, actor_addr from, channel to,
                    message_id mid, message data) {
    timer_->enqueue(invalid_actor_addr, invalid_message_id,
                     make_message(duration{rel_time}, std::move(from),
                                  std::move(to), mid, std::move(data)),
                     nullptr);
  }

  inline size_t num_workers() const {
    return num_workers_;
  }

protected:
  abstract_coordinator();

  virtual void initialize();

  virtual void stop() = 0;

  void stop_actors();

  // Creates a default instance.
  static abstract_coordinator* create_singleton();

  inline void dispose() {
    delete this;
  }

  actor timer_;
  actor printer_;

  // ID of the worker receiving the next enqueue
  std::atomic<size_t> next_worker_;

  size_t num_workers_;
};

} // namespace scheduler
} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_SCHEDULER_ABSTRACT_COORDINATOR_HPP
