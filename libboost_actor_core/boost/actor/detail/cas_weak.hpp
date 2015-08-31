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

#ifndef BOOST_ACTOR_DETAIL_CAS_WEAK_HPP
#define BOOST_ACTOR_DETAIL_CAS_WEAK_HPP

#include <atomic>

#include "boost/actor/config.hpp"

namespace boost {
namespace actor {
namespace detail {

template<class T>
bool cas_weak(std::atomic<T>* obj, T* expected, T desired) {
# if (defined(BOOST_ACTOR_CLANG) && BOOST_ACTOR_COMPILER_VERSION < 30401)                      \
     || (defined(BOOST_ACTOR_GCC) && BOOST_ACTOR_COMPILER_VERSION < 40803)
  return std::atomic_compare_exchange_strong(obj, expected, desired);
# else
  return std::atomic_compare_exchange_weak(obj, expected, desired);
# endif
}

} // namespace detail
} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_DETAIL_CAS_WEAK_HPP
