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

#ifndef BOOST_ACTOR_SCOPED_ACTOR_HPP
#define BOOST_ACTOR_SCOPED_ACTOR_HPP

#include "boost/actor/behavior.hpp"
#include "boost/actor/blocking_actor.hpp"

namespace boost {
namespace actor {

/// A scoped handle to a blocking actor.
class scoped_actor {
public:
  scoped_actor();

  scoped_actor(const scoped_actor&) = delete;

  explicit scoped_actor(bool hide_actor);

  ~scoped_actor();

  inline blocking_actor* operator->() const {
    return self_.get();
  }

  inline blocking_actor& operator*() const {
    return *self_;
  }

  inline blocking_actor* get() const {
    return self_.get();
  }

  inline operator channel() const {
    return get();
  }

  inline operator actor() const {
    return get();
  }

  inline operator actor_addr() const {
    return get()->address();
  }

  inline actor_addr address() const {
    return get()->address();
  }

private:
  void init(bool hide_actor);
  actor_id prev_; // used for logging/debugging purposes only
  intrusive_ptr<blocking_actor> self_;
};

} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_SCOPED_ACTOR_HPP
