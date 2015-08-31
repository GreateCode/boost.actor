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

#ifndef BOOST_ACTOR_DETAIL_SYNC_REQUEST_BOUNCER_HPP
#define BOOST_ACTOR_DETAIL_SYNC_REQUEST_BOUNCER_HPP

#include <cstdint>

namespace boost {
namespace actor {

class actor_addr;
class message_id;
class local_actor;
class mailbox_element;

} // namespace actor
} // namespace boost

namespace boost {
namespace actor {
namespace detail {

struct sync_request_bouncer {
  uint32_t rsn;
  explicit sync_request_bouncer(uint32_t r);
  void operator()(const actor_addr& sender, const message_id& mid) const;
  void operator()(const mailbox_element& e) const;

};

} // namespace detail
} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_DETAIL_SYNC_REQUEST_BOUNCER_HPP
