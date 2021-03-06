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

#include "boost/actor/detail/actor_registry.hpp"

#include <mutex>
#include <limits>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

#include "boost/actor/spawn.hpp"
#include "boost/actor/locks.hpp"
#include "boost/actor/exception.hpp"
#include "boost/actor/actor_cast.hpp"
#include "boost/actor/attachable.hpp"
#include "boost/actor/exit_reason.hpp"
#include "boost/actor/scoped_actor.hpp"
#include "boost/actor/stateful_actor.hpp"
#include "boost/actor/event_based_actor.hpp"

#include "boost/actor/experimental/announce_actor_type.hpp"

#include "boost/actor/scheduler/detached_threads.hpp"

#include "boost/actor/detail/logging.hpp"
#include "boost/actor/detail/shared_spinlock.hpp"

namespace boost {
namespace actor {
namespace detail {

namespace {

using exclusive_guard = unique_lock<shared_spinlock>;
using shared_guard = shared_lock<shared_spinlock>;

} // namespace <anonymous>

actor_registry::~actor_registry() {
  // nop
}

actor_registry::actor_registry() : running_(0) {
  // nop
}

actor_registry::value_type actor_registry::get_entry(actor_id key) const {
  shared_guard guard(instances_mtx_);
  auto i = entries_.find(key);
  if (i != entries_.end()) {
    return i->second;
  }
  BOOST_ACTOR_LOG_DEBUG("key not found, assume the actor no longer exists: " << key);
  return {nullptr, exit_reason::unknown};
}

void actor_registry::put(actor_id key, const abstract_actor_ptr& val) {
  if (val == nullptr) {
    return;
  }
  { // lifetime scope of guard
    exclusive_guard guard(instances_mtx_);
    if (! entries_.emplace(key,
                           value_type{val, exit_reason::not_exited}).second) {
      // already defined
      return;
    }
  }
  // attach functor without lock
  BOOST_ACTOR_LOG_INFO("added actor with ID " << key);
  actor_registry* reg = this;
  val->attach_functor([key, reg](uint32_t reason) { reg->erase(key, reason); });
}

void actor_registry::put(actor_id key, const actor_addr& value) {
  auto ptr = actor_cast<abstract_actor_ptr>(value);
  put(key, ptr);
}

void actor_registry::erase(actor_id key, uint32_t reason) {
  exclusive_guard guard(instances_mtx_);
  auto i = entries_.find(key);
  if (i != entries_.end()) {
    auto& entry = i->second;
    BOOST_ACTOR_LOG_INFO("erased actor with ID " << key << ", reason " << reason);
    entry.first = nullptr;
    entry.second = reason;
  }
}

void actor_registry::inc_running() {
# if defined(BOOST_ACTOR_LOG_LEVEL) && BOOST_ACTOR_LOG_LEVEL >= BOOST_ACTOR_DEBUG
    BOOST_ACTOR_LOG_DEBUG("new value = " << ++running_);
# else
    ++running_;
# endif
}

size_t actor_registry::running() const {
  return running_.load();
}

void actor_registry::dec_running() {
  size_t new_val = --running_;
  if (new_val <= 1) {
    std::unique_lock<std::mutex> guard(running_mtx_);
    running_cv_.notify_all();
  }
  BOOST_ACTOR_LOG_DEBUG(BOOST_ACTOR_ARG(new_val));
}

void actor_registry::await_running_count_equal(size_t expected) {
  BOOST_ACTOR_ASSERT(expected == 0 || expected == 1);
  BOOST_ACTOR_LOG_TRACE(BOOST_ACTOR_ARG(expected));
  std::unique_lock<std::mutex> guard{running_mtx_};
  while (running_ != expected) {
    BOOST_ACTOR_LOG_DEBUG("count = " << running_.load());
    running_cv_.wait(guard);
  }
}

actor actor_registry::get_named(atom_value key) const {
  shared_guard guard{named_entries_mtx_};
  auto i = named_entries_.find(key);
  if (i == named_entries_.end())
    return invalid_actor;
  return i->second;
}

void actor_registry::put_named(atom_value key, actor value) {
  if (value)
    value->attach_functor([=](uint32_t) {
      detail::singletons::get_actor_registry()->put_named(key, invalid_actor);
    });
  exclusive_guard guard{named_entries_mtx_};
  named_entries_.emplace(key, std::move(value));
}

auto actor_registry::named_actors() const -> named_entries {
  shared_guard guard{named_entries_mtx_};
  return named_entries_;
}

actor_registry* actor_registry::create_singleton() {
  return new actor_registry;
}

void actor_registry::dispose() {
  delete this;
}

void actor_registry::stop() {
  scoped_actor self{true};
  try {
    for (auto& kvp : named_entries_) {
      self->monitor(kvp.second);
      self->send_exit(kvp.second, exit_reason::kill);
      self->receive(
        [](const down_msg&) {
          // nop
        }
      );
    }
  }
  catch (actor_exited&) {
    BOOST_ACTOR_LOG_ERROR("actor_exited thrown in actor_registry::stop");
  }
  named_entries_.clear();
}

void actor_registry::initialize() {
  using namespace experimental;
  struct kvstate {
    using key_type = std::string;
    using mapped_type = message;
    using subscriber_set = std::unordered_set<actor>;
    using topic_set = std::unordered_set<std::string>;
    std::unordered_map<key_type, std::pair<mapped_type, subscriber_set>> data;
    std::unordered_map<actor,topic_set> subscribers;
    const char* name = "caf.config_server";
  };
  auto kvstore = [](stateful_actor<kvstate>* self) -> behavior {
    return {
      [=](put_atom, const std::string& key, message& msg) {
        auto& vp = self->state.data[key];
        if (vp.first == msg)
          return;
        vp.first = std::move(msg);
        for (auto& subscriber : vp.second)
          if (subscriber != self->current_sender())
            self->send(subscriber, update_atom::value, key, vp.second);
      },
      [=](get_atom, std::string& key) -> message {
        auto i = self->state.data.find(key);
        return make_message(ok_atom::value, std::move(key),
                            i != self->state.data.end() ? i->second.first
                                                        : make_message());
      },
      [=](subscribe_atom, const std::string& key) {
        auto subscriber = actor_cast<actor>(self->current_sender());
        if (! subscriber)
          return;
        self->state.data[key].second.insert(subscriber);
        auto& subscribers = self->state.subscribers;
        auto i = subscribers.find(subscriber);
        if (i != subscribers.end()) {
          i->second.insert(key);
        } else {
          self->monitor(subscriber);
          subscribers.emplace(subscriber, kvstate::topic_set{key});
        }
      },
      [=](unsubscribe_atom, const std::string& key) {
        auto subscriber = actor_cast<actor>(self->current_sender());
        if (! subscriber)
          return;
        self->state.subscribers[subscriber].erase(key);
        self->state.data[key].second.erase(subscriber);
      },
      [=](const down_msg& dm) {
        auto subscriber = actor_cast<actor>(dm.source);
        if (! subscriber)
          return;
        auto& subscribers = self->state.subscribers;
        auto i = subscribers.find(subscriber);
        if (i == subscribers.end())
          return;
        for (auto& key : i->second)
          self->state.data[key].second.erase(subscriber);
        subscribers.erase(i);
      },
      others >> [] {
        return make_message(error_atom::value, "unsupported operation");
      }
    };
  };
  // NOTE: all actors that we spawn here *must not* access the scheduler,
  //       because the scheduler will make sure that the registry is running
  //       as part of its initialization; hence, all actors have to
  //       use the lazy_init flag
  named_entries_.emplace(atom("SpawnServ"), spawn_announce_actor_type_server());
  named_entries_.emplace(atom("ConfigServ"), spawn<hidden+lazy_init>(kvstore));
}

} // namespace detail
} // namespace actor
} // namespace boost
