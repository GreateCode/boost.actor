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

#ifndef BOOST_ACTOR_DETAIL_SPLIT_JOIN_HPP
#define BOOST_ACTOR_DETAIL_SPLIT_JOIN_HPP

#include <vector>

#include "boost/actor/actor.hpp"
#include "boost/actor/locks.hpp"
#include "boost/actor/spawn.hpp"
#include "boost/actor/event_based_actor.hpp"

#include "boost/actor/detail/shared_spinlock.hpp"

namespace boost {
namespace actor {
namespace detail {

using actor_msg_vec = std::vector<std::pair<actor, message>>;

template <class T, class Split, class Join>
class split_join_collector : public event_based_actor {
public:
  split_join_collector(T init_value, Split s, Join j, actor_msg_vec xs)
      : workset_(std::move(xs)),
        awaited_results_(workset_.size()),
        join_(std::move(j)),
        split_(std::move(s)),
        value_(std::move(init_value)) {
    // nop
  }

  behavior make_behavior() override {
    return {
      // first message is the forwarded request
      others >> [=] {
        auto rp = this->make_response_promise();
        split_(workset_, this->current_message());
        for (auto& x : workset_)
          this->send(x.first, std::move(x.second));
        this->become(
          // collect results
          others >> [=] {
            join_(value_, this->current_message());
            if (--awaited_results_ == 0) {
              rp.deliver(make_message(value_));
              quit();
            }
          }
        );
        // no longer needed
        workset_.clear();
      }
    };
  }

private:
  actor_msg_vec workset_;
  size_t awaited_results_;
  Join join_;
  Split split_;
  T value_;
};

struct nop_split {
  inline void operator()(actor_msg_vec& xs, message& y) const {
    for (auto& x : xs) {
      x.second = y;
    }
  }
};

template <class T, class Split, class Join>
class split_join {
public:
  split_join(T init_value, Split s, Join j)
      : init_(std::move(init_value)),
        sf_(std::move(s)),
        jf_(std::move(j)) {
    // nop
  }

  void operator()(upgrade_lock<detail::shared_spinlock>& ulock,
                  const std::vector<actor>& workers,
                  mailbox_element_ptr& ptr,
                  execution_unit* host) {
    if (ptr->sender == invalid_actor_addr) {
      return;
    }
    actor_msg_vec xs(workers.size());
    for (size_t i = 0; i < workers.size(); ++i) {
      xs[i].first = workers[i];
    }
    ulock.unlock();
    using collector_t = split_join_collector<T, Split, Join>;
    auto hdl = spawn<collector_t, lazy_init>(init_, sf_, jf_, std::move(xs));
    hdl->enqueue(std::move(ptr), host);
  }

private:
  T init_;
  Split sf_; // split function
  Join jf_;  // join function
};

} // namespace detail
} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_DETAIL_SPLIT_JOIN_HPP

