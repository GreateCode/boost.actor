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

#ifndef BOOST_ACTOR_IO_MIDDLEMAN_HPP
#define BOOST_ACTOR_IO_MIDDLEMAN_HPP

#include <map>
#include <vector>
#include <memory>
#include <thread>

#include "boost/actor/fwd.hpp"
#include "boost/actor/node_id.hpp"
#include "boost/actor/actor_namespace.hpp"
#include "boost/actor/detail/singletons.hpp"

#include "boost/actor/io/hook.hpp"
#include "boost/actor/io/broker.hpp"
#include "boost/actor/io/middleman_actor.hpp"
#include "boost/actor/io/network/multiplexer.hpp"

namespace boost {
namespace actor {
namespace io {

/// Manages brokers and network backends.
class middleman : public detail::abstract_singleton {
public:
  friend class detail::singletons;

  ~middleman();

  /// Get middleman instance.
  static middleman* instance();

  /// Returns a handle to the actor managing the middleman singleton.
  middleman_actor actor_handle();

  /// Returns the broker associated with `name` or creates a
  /// new instance of type `Impl`.
  template <class Impl>
  intrusive_ptr<Impl> get_named_broker(atom_value name) {
    auto i = named_brokers_.find(name);
    if (i != named_brokers_.end())
      return static_cast<Impl*>(i->second.get());
    auto result = make_counted<Impl>(*this);
    BOOST_ACTOR_ASSERT(result->unique());
    result->launch(nullptr, false, true);
    named_brokers_.emplace(name, result);
    return result;
  }

  /// Adds `bptr` to the list of known brokers.
  void add_broker(broker_ptr bptr);

  /// Runs `fun` in the event loop of the middleman.
  /// @note This member function is thread-safe.
  template <class F>
  void run_later(F fun) {
    backend_->post(fun);
  }

  /// Returns the IO backend used by this middleman.
  inline network::multiplexer& backend() {
    return *backend_;
  }

  /// Invokes the callback(s) associated with given event.
  template <hook::event_type Event, typename... Ts>
  void notify(Ts&&... ts) {
    if (hooks_) {
      hooks_->handle<Event>(std::forward<Ts>(ts)...);
    }
  }

  /// Adds a new hook to the middleman.
  template<class C, typename... Ts>
  void add_hook(Ts&&... xs) {
    // if only we could move a unique_ptr into a lambda in C++11
    auto ptr = new C(std::forward<Ts>(xs)...);
    backend().dispatch([=] {
      ptr->next.swap(hooks_);
      hooks_.reset(ptr);
    });
  }

  inline bool has_hook() const {
    return hooks_ != nullptr;
  }

  template <class F>
  void add_shutdown_cb(F fun) {
    struct impl : hook {
      impl(F&& f) : fun_(std::move(f)) {
        // nop
      }

      void before_shutdown_cb() override {
        fun_();
        call_next<hook::before_shutdown>();
      }

      F fun_;
    };
    add_hook<impl>(std::move(fun));
  }

  /// @cond PRIVATE

  using backend_pointer = std::unique_ptr<network::multiplexer>;

  using backend_factory = std::function<backend_pointer()>;

  // stops the singleton
  void stop() override;

  // deletes the singleton
  void dispose() override;

  // initializes the singleton
  void initialize() override;

  middleman(const backend_factory&);

  /// @endcond

private:
  // networking backend
  backend_pointer backend_;
  // prevents backend from shutting down unless explicitly requested
  network::multiplexer::supervisor_ptr backend_supervisor_;
  // runs the backend
  std::thread thread_;
  // keeps track of "singleton-like" brokers
  std::map<atom_value, broker_ptr> named_brokers_;
  // keeps track of anonymous brokers
  std::set<broker_ptr> brokers_;
  // user-defined hooks
  hook_uptr hooks_;
  // actor offering asyncronous IO by managing this singleton instance
  middleman_actor manager_;
};

} // namespace io
} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_IO_MIDDLEMAN_HPP
