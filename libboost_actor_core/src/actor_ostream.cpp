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

#include "boost/actor/actor_ostream.hpp"

#include "boost/actor/all.hpp"
#include "boost/actor/local_actor.hpp"
#include "boost/actor/scoped_actor.hpp"

#include "boost/actor/scheduler/abstract_coordinator.hpp"

#include "boost/actor/detail/singletons.hpp"

namespace boost {
namespace actor {

actor_ostream::actor_ostream(actor self)
    : self_(std::move(self)),
      printer_(detail::singletons::get_scheduling_coordinator()->printer()) {
  // nop
}

actor_ostream& actor_ostream::write(std::string arg) {
  send_as(self_, printer_, add_atom::value, std::move(arg));
  return *this;
}

actor_ostream& actor_ostream::flush() {
  send_as(self_, printer_, flush_atom::value);
  return *this;
}

void actor_ostream::redirect(const actor& src, std::string f, int flags) {
  send_as(src, detail::singletons::get_scheduling_coordinator()->printer(),
          redirect_atom::value, src.address(), std::move(f), flags);
}

void actor_ostream::redirect_all(std::string f, int flags) {
  anon_send(detail::singletons::get_scheduling_coordinator()->printer(),
            redirect_atom::value, std::move(f), flags);
}

} // namespace actor
} // namespace boost

namespace std {

boost::actor::actor_ostream& endl(boost::actor::actor_ostream& o) {
  return o.write("\n");
}

boost::actor::actor_ostream& flush(boost::actor::actor_ostream& o) {
  return o.flush();
}

} // namespace std
