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
#include "boost/actor/message.hpp"
#include "boost/actor/abstract_group.hpp"
#include "boost/actor/detail/shared_spinlock.hpp"

#include "boost/actor/detail/group_manager.hpp"
#include "boost/actor/detail/singletons.hpp"

namespace boost {
namespace actor {

abstract_group::subscription::subscription(const abstract_group_ptr& g)
    : group_(g) {
  // nop
}

void abstract_group::subscription::actor_exited(abstract_actor* ptr, uint32_t) {
  group_->unsubscribe(ptr->address());
}

bool abstract_group::subscription::matches(const token& what) {
  if (what.subtype != attachable::token::subscription) {
    return false;
  }
  if (what.ptr) {
    auto& ot = *reinterpret_cast<const subscription_token*>(what.ptr);
    return ot.group == group_;
  }
  return true;
}

abstract_group::module::module(std::string mname) : name_(std::move(mname)) {
  // nop
}

void abstract_group::module::stop() {
  // nop
}

const std::string& abstract_group::module::name() const {
  return name_;
}

abstract_group::abstract_group(abstract_group::module_ptr mod,
                               std::string id, const node_id& nid)
    : abstract_channel(abstract_channel::is_abstract_group_flag, nid),
      module_(mod),
      identifier_(std::move(id)) {
  // nop
}

const std::string& abstract_group::identifier() const {
  return identifier_;
}

abstract_group::module_ptr abstract_group::get_module() const {
  return module_;
}

const std::string& abstract_group::module_name() const {
  return get_module()->name();
}

abstract_group::module::~module() {
  // nop
}

abstract_group::~abstract_group() {
  // nop
}

} // namespace actor
} // namespace boost
