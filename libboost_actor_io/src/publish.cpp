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

#include "boost/actor/io/publish.hpp"

#include "boost/actor/send.hpp"
#include "boost/actor/exception.hpp"
#include "boost/actor/actor_cast.hpp"
#include "boost/actor/scoped_actor.hpp"
#include "boost/actor/abstract_actor.hpp"

#include "boost/actor/detail/logging.hpp"
#include "boost/actor/detail/singletons.hpp"
#include "boost/actor/detail/actor_registry.hpp"

#include "boost/actor/io/basp_broker.hpp"
#include "boost/actor/io/middleman_actor.hpp"

namespace boost {
namespace actor {
namespace io {

uint16_t publish_impl(uint16_t port, actor_addr whom,
                      std::set<std::string> sigs, const char* in, bool ru) {
  if (whom == invalid_actor_addr) {
    throw network_error("cannot publish an invalid actor");
  }
  BOOST_ACTOR_LOGF_TRACE("whom = " << to_string(whom->address())
                 << ", " << BOOST_ACTOR_ARG(port)
                 << ", in = " << (in ? in : "")
                 << ", " << BOOST_ACTOR_ARG(ru));
  std::string str;
  if (in != nullptr) {
    str = in;
  }
  auto mm = get_middleman_actor();
  scoped_actor self;
  uint16_t result;
  std::string error_msg;
  try {
    self->sync_send(mm, publish_atom::value, port,
                    std::move(whom), std::move(sigs), str, ru).await(
      [&](ok_atom, uint16_t res) {
        result = res;
      },
      [&](error_atom, std::string& msg) {
        if (! msg.empty())
          error_msg.swap(msg);
        else
          error_msg = "an unknown error occurred in the middleman";
      }
    );
  }
  catch (actor_exited& e) {
    error_msg = "scoped actor in boost::actor::publish quit unexpectedly: ";
    error_msg += e.what();
  }
  if (! error_msg.empty()) {
    throw network_error(std::move(error_msg));
  }
  return result;
}

} // namespace io
} // namespace actor
} // namespace boost
