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

#ifndef BOOST_ACTOR_MAILBOX_ELEMENT_HPP
#define BOOST_ACTOR_MAILBOX_ELEMENT_HPP

#include <cstddef>

#include "boost/actor/extend.hpp"
#include "boost/actor/message.hpp"
#include "boost/actor/actor_addr.hpp"
#include "boost/actor/message_id.hpp"
#include "boost/actor/ref_counted.hpp"

#include "boost/actor/detail/memory.hpp"
#include "boost/actor/detail/embedded.hpp"
#include "boost/actor/detail/disposer.hpp"
#include "boost/actor/detail/tuple_vals.hpp"
#include "boost/actor/detail/pair_storage.hpp"
#include "boost/actor/detail/message_data.hpp"
#include "boost/actor/detail/memory_cache_flag_type.hpp"

namespace boost {
namespace actor {

class mailbox_element : public memory_managed {
public:
  static constexpr auto memory_cache_flag = detail::needs_embedding;

  mailbox_element* next; // intrusive next pointer
  mailbox_element* prev; // intrusive previous pointer
  bool marked;           // denotes if this node is currently processed
  actor_addr sender;
  message_id mid;
  message msg;           // 'content field'

  mailbox_element();
  mailbox_element(actor_addr sender, message_id id);
  mailbox_element(actor_addr sender, message_id id, message data);

  ~mailbox_element();

  mailbox_element(mailbox_element&&) = delete;
  mailbox_element(const mailbox_element&) = delete;
  mailbox_element& operator=(mailbox_element&&) = delete;
  mailbox_element& operator=(const mailbox_element&) = delete;

  using unique_ptr = std::unique_ptr<mailbox_element, detail::disposer>;

  static unique_ptr make(actor_addr sender, message_id id, message msg);

  template <class... Ts>
  static unique_ptr make_joint(actor_addr sender, message_id id, Ts&&... xs) {
    using value_storage =
      detail::tuple_vals<
        typename unbox_message_element<
          typename detail::strip_and_convert<Ts>::type
        >::type...
      >;
    std::integral_constant<size_t, 2> tk;
    using storage = detail::pair_storage<mailbox_element, value_storage>;
    auto ptr = detail::memory::create<storage>(tk, std::move(sender), id,
                                               std::forward<Ts>(xs)...);
    ptr->first.msg.reset(&(ptr->second), false);
    return unique_ptr{&(ptr->first)};
  }

  inline bool is_high_priority() const {
    return mid.is_high_priority();
  }
};

using mailbox_element_ptr = std::unique_ptr<mailbox_element, detail::disposer>;

} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_MAILBOX_ELEMENT_HPP
