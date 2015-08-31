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

#ifndef BOOST_ACTOR_DETAIL_UNIFORM_TYPE_INFO_MAP_HPP
#define BOOST_ACTOR_DETAIL_UNIFORM_TYPE_INFO_MAP_HPP

#include <set>
#include <map>
#include <string>
#include <utility>
#include <type_traits>

#include "boost/actor/fwd.hpp"

#include "boost/actor/atom.hpp"
#include "boost/actor/unit.hpp"
#include "boost/actor/node_id.hpp"
#include "boost/actor/duration.hpp"
#include "boost/actor/system_messages.hpp"

#include "boost/actor/detail/type_list.hpp"

#include "boost/actor/detail/singleton_mixin.hpp"

namespace boost {
namespace actor {
class uniform_type_info;
} // namespace actor
} // namespace boost

namespace boost {
namespace actor {
namespace detail {

class uniform_type_info_map {
public:
  using pointer = const uniform_type_info*;

  virtual ~uniform_type_info_map();

  virtual pointer by_uniform_name(const std::string& name) = 0;

  virtual pointer by_type_nr(uint16_t) const = 0;

  virtual pointer by_rtti(const std::type_info& ti) const = 0;

  virtual std::vector<pointer> get_all() const = 0;

  virtual pointer insert(const std::type_info*, uniform_type_info_ptr) = 0;

  static uniform_type_info_map* create_singleton();

  void dispose();

  void stop();

  virtual void initialize() = 0;
};

} // namespace detail
} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_DETAIL_UNIFORM_TYPE_INFO_MAP_HPP
