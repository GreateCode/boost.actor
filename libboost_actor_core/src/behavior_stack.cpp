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

#include <iterator>

#include "boost/none.hpp"
#include "boost/actor/local_actor.hpp"
#include "boost/actor/detail/behavior_stack.hpp"

namespace boost {
namespace actor {
namespace detail {

void behavior_stack::pop_back() {
  if (elements_.empty()) {
    return;
  }
  erased_elements_.push_back(std::move(elements_.back()));
  elements_.pop_back();
}

void behavior_stack::clear() {
  if (! elements_.empty()) {
    if (erased_elements_.empty()) {
      elements_.swap(erased_elements_);
    } else {
      std::move(elements_.begin(), elements_.end(),
                std::back_inserter(erased_elements_));
      elements_.clear();
    }
  }
}

} // namespace detail
} // namespace actor
} // namespace boost
