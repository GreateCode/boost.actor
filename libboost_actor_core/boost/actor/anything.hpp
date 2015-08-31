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

#ifndef BOOST_ACTOR_ANYTHING_HPP
#define BOOST_ACTOR_ANYTHING_HPP

#include <type_traits>

namespace boost {
namespace actor {

/// Acts as wildcard expression in patterns.
struct anything {
  constexpr anything() {
    // nop
  }
};

/// @relates anything
inline bool operator==(const anything&, const anything&) {
  return true;
}

/// @relates anything
inline bool operator!=(const anything&, const anything&) {
  return false;
}

/// @relates anything
template <class T>
struct is_anything : std::is_same<T, anything> {
  // no content
};

} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_ANYTHING_HPP