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

#ifndef BOOST_ACTOR_DETAIL_RAW_ACCESS_HPP
#define BOOST_ACTOR_DETAIL_RAW_ACCESS_HPP

#include "boost/actor/actor.hpp"
#include "boost/actor/group.hpp"
#include "boost/actor/channel.hpp"
#include "boost/actor/actor_addr.hpp"
#include "boost/actor/abstract_actor.hpp"
#include "boost/actor/abstract_group.hpp"
#include "boost/actor/abstract_channel.hpp"

namespace boost {
namespace actor {
namespace detail {

class raw_access {

public:

  template <class ActorHandle>
  static abstract_actor* get(const ActorHandle& hdl) {
    return hdl.ptr_.get();
  }

  static abstract_channel* get(const channel& hdl) {
    return hdl.ptr_.get();
  }

  static abstract_group* get(const group& hdl) {
    return hdl.ptr_.get();
  }

  static actor unsafe_cast(abstract_actor* ptr) {
    return {ptr};
  }

  static actor unsafe_cast(const actor_addr& hdl) {
    return {get(hdl)};
  }

  static actor unsafe_cast(const abstract_actor_ptr& ptr) {
    return {ptr.get()};
  }

  template <class T>
  static void unsafe_assign(T& lhs, const actor& rhs) {
    lhs = T{get(rhs)};
  }

  template <class T>
  static void unsafe_assign(T& lhs, const abstract_actor_ptr& ptr) {
    lhs = T{ptr.get()};
  }

};

} // namespace detail
} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_DETAIL_RAW_ACCESS_HPP
