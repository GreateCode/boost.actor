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

#ifndef BOOST_ACTOR_TYPED_EVENT_BASED_ACTOR_HPP
#define BOOST_ACTOR_TYPED_EVENT_BASED_ACTOR_HPP

#include "boost/actor/replies_to.hpp"
#include "boost/actor/local_actor.hpp"
#include "boost/actor/typed_behavior.hpp"
#include "boost/actor/abstract_event_based_actor.hpp"

#include "boost/actor/mixin/sync_sender.hpp"

namespace boost {
namespace actor {

/// A cooperatively scheduled, event-based actor implementation with strong type
/// checking. This is the recommended base class for user-defined actors and is
/// used implicitly when spawning typed, functor-based actors without the
/// `blocking_api` flag.
/// @extends local_actor
template <class... Sigs>
class typed_event_based_actor
    : public abstract_event_based_actor<typed_behavior<Sigs...>, true> {
public:
  using base_type =
    abstract_event_based_actor<typed_behavior<Sigs...>, true>;

  using signatures = detail::type_list<Sigs...>;

  using behavior_type = typed_behavior<Sigs...>;

  std::set<std::string> message_types() const override {
    return {Sigs::static_type_name()...};
  }

  void initialize() override {
    this->is_initialized(true);
    auto bhvr = make_behavior();
    BOOST_ACTOR_LOG_DEBUG_IF(! bhvr, "make_behavior() did not return a behavior, "
                            << "has_behavior() = "
                            << std::boolalpha << this->has_behavior());
    if (bhvr) {
      // make_behavior() did return a behavior instead of using become()
      BOOST_ACTOR_LOG_DEBUG("make_behavior() did return a valid behavior");
      this->do_become(std::move(bhvr.unbox()), true);
    }
  }

  using base_type::send;
  using base_type::delayed_send;

  template <class... DestSigs, class... Ts>
  void send(message_priority mp, const typed_actor<DestSigs...>& dest, Ts&&... xs) {
    detail::sender_signature_checker<
      detail::type_list<Sigs...>,
      detail::type_list<DestSigs...>,
      detail::type_list<
        typename detail::implicit_conversions<
          typename std::decay<Ts>::type
        >::type...
      >
    >::check();
    base_type::send(mp, dest, std::forward<Ts>(xs)...);
  }

  template <class... DestSigs, class... Ts>
  void send(const typed_actor<DestSigs...>& dest, Ts&&... xs) {
    detail::sender_signature_checker<
      detail::type_list<Sigs...>,
      detail::type_list<DestSigs...>,
      detail::type_list<
        typename detail::implicit_conversions<
          typename std::decay<Ts>::type
        >::type...
      >
    >::check();
    base_type::send(dest, std::forward<Ts>(xs)...);
  }

  template <class... DestSigs, class... Ts>
  void delayed_send(message_priority mp, const typed_actor<DestSigs...>& dest,
                    const duration& rtime, Ts&&... xs) {
    detail::sender_signature_checker<
      detail::type_list<Sigs...>,
      detail::type_list<DestSigs...>,
      detail::type_list<
        typename detail::implicit_conversions<
          typename std::decay<Ts>::type
        >::type...
      >
    >::check();
    base_type::delayed_send(mp, dest, rtime, std::forward<Ts>(xs)...);
  }

  template <class... DestSigs, class... Ts>
  void delayed_send(const typed_actor<DestSigs...>& dest,
                    const duration& rtime, Ts&&... xs) {
    detail::sender_signature_checker<
      detail::type_list<Sigs...>,
      detail::type_list<DestSigs...>,
      detail::type_list<
        typename detail::implicit_conversions<
          typename std::decay<Ts>::type
        >::type...
      >
    >::check();
    base_type::delayed_send(dest, rtime, std::forward<Ts>(xs)...);
  }

protected:
  virtual behavior_type make_behavior() {
    if (this->initial_behavior_fac_) {
      auto bhvr = this->initial_behavior_fac_(this);
      this->initial_behavior_fac_ = nullptr;
      if (bhvr)
        this->do_become(std::move(bhvr), true);
    }
    return behavior_type::make_empty_behavior();
  }
};

} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_TYPED_EVENT_BASED_ACTOR_HPP
