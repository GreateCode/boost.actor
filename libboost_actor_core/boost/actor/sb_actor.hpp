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

#ifndef BOOST_ACTOR_SB_ACTOR_HPP
#define BOOST_ACTOR_SB_ACTOR_HPP

#include <utility>
#include <type_traits>

#include "boost/actor/config.hpp"
#include "boost/actor/event_based_actor.hpp"

namespace boost {
namespace actor {

// <backward_compatibility version="0.13">
template <class Derived, class Base = event_based_actor>
class sb_actor : public Base {
public:
  static_assert(std::is_base_of<event_based_actor, Base>::value,
                "Base must be event_based_actor or a derived type");

  behavior make_behavior() override {
    return static_cast<Derived*>(this)->init_state;
  }

protected:
  using combined_type = sb_actor;

  template <class... Ts>
  sb_actor(Ts&&... xs) : Base(std::forward<Ts>(xs)...) {
    // nop
  }
} BOOST_ACTOR_DEPRECATED ;
// </backward_compatibility>

} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_SB_ACTOR_HPP
