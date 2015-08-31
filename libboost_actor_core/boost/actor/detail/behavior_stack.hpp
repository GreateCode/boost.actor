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

#ifndef BOOST_ACTOR_DETAIL_BEHAVIOR_STACK_HPP
#define BOOST_ACTOR_DETAIL_BEHAVIOR_STACK_HPP

#include <vector>
#include <memory>
#include <utility>
#include <algorithm>

#include "boost/optional.hpp"

#include "boost/actor/config.hpp"
#include "boost/actor/behavior.hpp"
#include "boost/actor/message_id.hpp"
#include "boost/actor/mailbox_element.hpp"

namespace boost {
namespace actor {
namespace detail {

struct behavior_stack_mover;

class behavior_stack {
public:
  friend struct behavior_stack_mover;

  behavior_stack(const behavior_stack&) = delete;
  behavior_stack& operator=(const behavior_stack&) = delete;

  behavior_stack() = default;

  // erases the last (asynchronous) behavior
  void pop_back();

  void clear();

  inline bool empty() const {
    return elements_.empty();
  }

  inline behavior& back() {
    BOOST_ACTOR_ASSERT(! empty());
    return elements_.back();
  }

  inline void push_back(behavior&& what) {
    elements_.emplace_back(std::move(what));
  }

  inline void cleanup() {
    erased_elements_.clear();
  }

private:
  std::vector<behavior> elements_;
  std::vector<behavior> erased_elements_;
};

} // namespace detail
} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_DETAIL_BEHAVIOR_STACK_HPP
