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

#ifndef BOOST_ACTOR_DETAIL_OPTIONAL_MESSAGE_VISITOR_HPP
#define BOOST_ACTOR_DETAIL_OPTIONAL_MESSAGE_VISITOR_HPP

#include "boost/none.hpp"
#include "boost/actor/unit.hpp"
#include "boost/optional.hpp"
#include "boost/actor/skip_message.hpp"
#include "boost/actor/static_visitor.hpp"
#include "boost/actor/response_promise.hpp"
#include "boost/actor/typed_response_promise.hpp"

#include "boost/actor/detail/type_traits.hpp"

namespace boost {
namespace actor {
namespace detail {


template <class T>
struct is_message_id_wrapper {
  template <class U>
  static char (&test(typename U::message_id_wrapper_tag))[1];
  template <class U>
  static char (&test(...))[2];
  static constexpr bool value = sizeof(test<T>(0)) == 1;
};

template <class T>
struct is_response_promise : std::false_type { };

template <>
struct is_response_promise<response_promise> : std::true_type { };

template <class T>
struct is_response_promise<typed_response_promise<T>> : std::true_type { };

template <class T>
struct optional_message_visitor_enable_tpl {
  static constexpr bool value =
      ! is_one_of<
        typename std::remove_const<T>::type,
        none_t,
        unit_t,
        skip_message_t,
        optional<skip_message_t>
      >::value
      && ! is_message_id_wrapper<T>::value
      && ! is_response_promise<T>::value;
};

class optional_message_visitor : public static_visitor<optional<message>> {
public:
  optional_message_visitor() = default;

  using opt_msg = optional<message>;

  inline opt_msg operator()(const none_t&) const {
    return none;
  }

  inline opt_msg operator()(const skip_message_t&) const {
    return none;
  }

  inline opt_msg operator()(const unit_t&) const {
    return message{};
  }

  inline opt_msg operator()(const optional<skip_message_t>& val) const {
    if (val) {
      return none;
    }
    return message{};
  }

  inline opt_msg operator()(const response_promise&) const {
    return message{};
  }

  template <class T>
  inline opt_msg operator()(const typed_response_promise<T>&) const {
    return message{};
  }

  template <class T, class... Ts>
  typename std::enable_if<
    optional_message_visitor_enable_tpl<T>::value,
    opt_msg
  >::type
  operator()(T& x, Ts&... xs) const {
    return make_message(std::move(x), std::move(xs)...);
  }

  template <class T>
  typename std::enable_if<
    is_message_id_wrapper<T>::value,
    opt_msg
  >::type
  operator()(T& value) const {
    return make_message(atom("MESSAGE_ID"),
              value.get_message_id().integer_value());
  }

  template <class L, class R>
  opt_msg operator()(either_or_t<L, R>& value) const {
    return std::move(value.value);
  }

  template <class... Ts>
  opt_msg operator()(std::tuple<Ts...>& value) const {
    return apply_args(*this, get_indices(value), value);
  }
};

} // namespace detail
} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_DETAIL_OPTIONAL_MESSAGE_VISITOR_HPP
