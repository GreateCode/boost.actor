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

#ifndef BOOST_ACTOR_ACTOR_PROXY_HPP
#define BOOST_ACTOR_ACTOR_PROXY_HPP

#include <atomic>
#include <cstdint>

#include "boost/actor/abstract_actor.hpp"

#include "boost/actor/detail/shared_spinlock.hpp"

namespace boost {
namespace actor {

class actor_proxy;

/// A smart pointer to an {@link actor_proxy} instance.
/// @relates actor_proxy
using actor_proxy_ptr = intrusive_ptr<actor_proxy>;

/// Represents an actor running on a remote machine,
/// or different hardware, or in a separate process.
class actor_proxy : public abstract_actor {
public:
  /// An anchor points to a proxy instance without sharing
  /// ownership to it, i.e., models a weak ptr.
  class anchor : public ref_counted {
  public:
    friend class actor_proxy;

    anchor(actor_proxy* instance = nullptr);

    ~anchor();

    /// Queries whether the proxy was already deleted.
    bool expired() const;

    /// Gets a pointer to the proxy or `nullptr`
    ///    if the instance is {@link expired()}.
    actor_proxy_ptr get();

  private:
    /*
     * Tries to expire this anchor. Fails if reference
     *    count of the proxy is nonzero.
     */
    bool try_expire() noexcept;
    std::atomic<actor_proxy*> ptr_;
    detail::shared_spinlock lock_;
  };

  using anchor_ptr = intrusive_ptr<anchor>;

  ~actor_proxy();

  /// Establishes a local link state that's not synchronized back
  ///    to the remote instance.
  virtual void local_link_to(const actor_addr& other) = 0;

  /// Removes a local link state.
  virtual void local_unlink_from(const actor_addr& other) = 0;

  /// Invokes cleanup code.
  virtual void kill_proxy(uint32_t reason) = 0;

  void request_deletion(bool decremented_ref_count) noexcept override;

  inline anchor_ptr get_anchor() const {
    return anchor_;
  }

protected:
  actor_proxy(actor_id aid, node_id nid);
  anchor_ptr anchor_;
};

} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_ACTOR_PROXY_HPP
