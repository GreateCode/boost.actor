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

#include "boost/actor/all.hpp"

#include "boost/actor/io/publish.hpp"
#include "boost/actor/io/middleman.hpp"
#include "boost/actor/io/publish_local_groups.hpp"

namespace boost {
namespace actor {
namespace io {

uint16_t publish_local_groups(uint16_t port, const char* addr) {
  auto group_nameserver = []() -> behavior {
    return {
      [](get_atom, const std::string& name) {
        return group::get("local", name);
      }
    };
  };
  auto gn = spawn<hidden>(group_nameserver);
  uint16_t result;
  try {
    result = publish(gn, port, addr);
  }
  catch (std::exception&) {
    anon_send_exit(gn, exit_reason::user_shutdown);
    throw;
  }
  middleman::instance()->add_shutdown_cb([gn] {
    anon_send_exit(gn, exit_reason::user_shutdown);
  });
  return result;
}

} // namespace io
} // namespace actor
} // namespace boost
