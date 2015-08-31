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

#ifndef BOOST_ACTOR_CHANNEL_HPP
#define BOOST_ACTOR_CHANNEL_HPP

#include <cstddef>
#include <type_traits>

#include "boost/intrusive_ptr.hpp"

#include "boost/actor/fwd.hpp"
#include "boost/actor/abstract_channel.hpp"

#include "boost/actor/detail/comparable.hpp"

namespace boost {
namespace actor {

class actor;
class group;
class execution_unit;

struct invalid_actor_t;
struct invalid_group_t;

/// A handle to instances of `abstract_channel`.
class channel : detail::comparable<channel>,
                detail::comparable<channel, actor>,
                detail::comparable<channel, abstract_channel*> {
public:
  template <class T, typename U>
  friend T actor_cast(const U&);

  channel() = default;

  channel(const actor&);

  channel(const group&);

  channel(const invalid_actor_t&);

  channel(const invalid_group_t&);

  template <class T>
  channel(intrusive_ptr<T> ptr,
          typename std::enable_if<
            std::is_base_of<abstract_channel, T>::value
          >::type* = 0)
      : ptr_(ptr) {
    // nop
  }

  channel(abstract_channel* ptr);

  inline explicit operator bool() const {
    return static_cast<bool>(ptr_);
  }

  inline bool operator!() const {
    return ! ptr_;
  }

  inline abstract_channel* operator->() const {
    return ptr_.get();
  }

  inline abstract_channel& operator*() const {
    return *ptr_;
  }

  intptr_t compare(const channel& other) const;

  intptr_t compare(const actor& other) const;

  intptr_t compare(const abstract_channel* other) const;

  static intptr_t compare(const abstract_channel* lhs,
                          const abstract_channel* rhs);

private:
  inline abstract_channel* get() const {
    return ptr_.get();
  }

  abstract_channel_ptr ptr_;
};

} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_CHANNEL_HPP
