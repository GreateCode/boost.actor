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

#include "boost/actor/io/unpublish.hpp"

#include "boost/actor/send.hpp"
#include "boost/actor/scoped_actor.hpp"

#include "boost/actor/io/unpublish.hpp"
#include "boost/actor/io/basp_broker.hpp"
#include "boost/actor/io/middleman_actor.hpp"

namespace boost {
namespace actor {
namespace io {

void unpublish_impl(const actor_addr& whom, uint16_t port, bool blocking) {
  BOOST_ACTOR_LOGF_TRACE(BOOST_ACTOR_TSARG(whom) << ", " << BOOST_ACTOR_ARG(port) << BOOST_ACTOR_ARG(blocking));
  auto mm = get_middleman_actor();
  if (blocking) {
    scoped_actor self;
    self->sync_send(mm, unpublish_atom::value, whom, port).await(
      [](ok_atom) {
        // ok, basp_broker is done
      },
      [](error_atom, const std::string&) {
        // ok, basp_broker is done
      }
    );
  } else {
    anon_send(mm, unpublish_atom::value, whom, port);
  }
}

} // namespace io
} // namespace actor
} // namespace boost
