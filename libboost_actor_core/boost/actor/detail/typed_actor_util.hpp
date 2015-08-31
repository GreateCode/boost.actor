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

#ifndef BOOST_ACTOR_DETAIL_TYPED_ACTOR_DETAIL_HPP
#define BOOST_ACTOR_DETAIL_TYPED_ACTOR_DETAIL_HPP

#include <tuple>

#include "boost/actor/delegated.hpp"
#include "boost/actor/replies_to.hpp"
#include "boost/actor/system_messages.hpp"

#include "boost/actor/detail/type_list.hpp"

namespace boost {
namespace actor { // forward declarations

template <class R>
class typed_continue_helper;

} // namespace actor
} // namespace boost

namespace boost {
namespace actor {
namespace detail {

template <class T>
struct unwrap_std_tuple {
  using type = type_list<T>;
};

template <class... Ts>
struct unwrap_std_tuple<std::tuple<Ts...>> {
  using type = type_list<Ts...>;
};

template <class T>
struct deduce_lhs_result {
  using type = typename unwrap_std_tuple<T>::type;
};

template <class L, class R>
struct deduce_lhs_result<either_or_t<L, R>> {
  using type = L;
};

template <class T>
struct deduce_rhs_result {
  using type = type_list<>;
};

template <class L, class R>
struct deduce_rhs_result<either_or_t<L, R>> {
  using type = R;
};

template <class T>
struct is_hidden_msg_handler : std::false_type { };

template <>
struct is_hidden_msg_handler<typed_mpi<type_list<exit_msg>,
                                       type_list<void>,
                                       empty_type_list>> : std::true_type { };

template <>
struct is_hidden_msg_handler<typed_mpi<type_list<down_msg>,
                                       type_list<void>,
                                       empty_type_list>> : std::true_type { };

template <class T>
struct deduce_mpi {
  using result = typename implicit_conversions<typename T::result_type>::type;
  using arg_t = typename tl_map<typename T::arg_types, std::decay>::type;
  using type = typed_mpi<arg_t,
                         typename deduce_lhs_result<result>::type,
                         typename deduce_rhs_result<result>::type>;
};

template <class Arguments>
struct input_is {
  template <class Signature>
  struct eval {
    static constexpr bool value =
      std::is_same<Arguments, typename Signature::input_types>::value;
  };
};

template <class OutputPair, class... Fs>
struct type_checker;

template <class OutputList, class F1>
struct type_checker<OutputList, F1> {
  static void check() {
    using arg_types =
      typename tl_map<
        typename get_callable_trait<F1>::arg_types,
        std::decay
      >::type;
    static_assert(std::is_same<OutputList, arg_types>::value
                  || (std::is_same<OutputList, type_list<void>>::value
                     && std::is_same<arg_types, type_list<>>::value),
                  "wrong functor signature");
  }
};

template <class Opt1, class Opt2, class F1>
struct type_checker<type_pair<Opt1, Opt2>, F1> {
  static void check() {
    type_checker<Opt1, F1>::check();
  }
};

template <class OutputPair, class F1>
struct type_checker<OutputPair, F1, none_t> : type_checker<OutputPair, F1> { };

template <class Opt1, class Opt2, class F1, class F2>
struct type_checker<type_pair<Opt1, Opt2>, F1, F2> {
  static void check() {
    type_checker<Opt1, F1>::check();
    type_checker<Opt2, F2>::check();
  }
};

template <int X, int Pos>
struct static_error_printer {
  static_assert(X != Pos, "unexpected handler some position > 20");

};

template <int X>
struct static_error_printer<X, -3> {
  static_assert(X == -1, "too few message handlers defined");
};

template <int X>
struct static_error_printer<X, -2> {
  static_assert(X == -1, "too many message handlers defined");
};

template <int X>
struct static_error_printer<X, -1> {
  // everything' fine
};

#define BOOST_ACTOR_STATICERR(Pos)                                                     \
  template <int X>                                                             \
  struct static_error_printer< X, Pos > {                                      \
    static_assert(X == -1, "unexpected handler at position " #Pos );           \
  }

BOOST_ACTOR_STATICERR( 0); BOOST_ACTOR_STATICERR( 1); BOOST_ACTOR_STATICERR( 2);
BOOST_ACTOR_STATICERR( 3); BOOST_ACTOR_STATICERR( 4); BOOST_ACTOR_STATICERR( 5);
BOOST_ACTOR_STATICERR( 6); BOOST_ACTOR_STATICERR( 7); BOOST_ACTOR_STATICERR( 8);
BOOST_ACTOR_STATICERR( 9); BOOST_ACTOR_STATICERR(10); BOOST_ACTOR_STATICERR(11);
BOOST_ACTOR_STATICERR(12); BOOST_ACTOR_STATICERR(13); BOOST_ACTOR_STATICERR(14);
BOOST_ACTOR_STATICERR(15); BOOST_ACTOR_STATICERR(16); BOOST_ACTOR_STATICERR(17);
BOOST_ACTOR_STATICERR(18); BOOST_ACTOR_STATICERR(19); BOOST_ACTOR_STATICERR(20);

template <class A, class B, template <class, class> class Predicate>
struct static_asserter {
  static void verify_match() {
    static constexpr int x = Predicate<A, B>::value;
    static_error_printer<x, x> dummy;
    static_cast<void>(dummy);
  }
};

template <class T>
struct lifted_result_type {
  using type = type_list<typename implicit_conversions<T>::type>;
};

template <class... Ts>
struct lifted_result_type<std::tuple<Ts...>> {
  using type = type_list<Ts...>;
};

template <class T>
struct deduce_lifted_output_type {
  using type = T;
};

template <class R>
struct deduce_lifted_output_type<type_list<typed_continue_helper<R>>> {
  using type = typename lifted_result_type<R>::type;
};

template <class Signatures, class InputTypes>
struct deduce_output_type {
  static constexpr int input_pos =
    tl_find_if<
      Signatures,
      input_is<InputTypes>::template eval
    >::value;
  static_assert(input_pos != -1, "typed actor does not support given input");
  using signature = typename tl_at<Signatures, input_pos>::type;
  using opt1 = typename signature::output_opt1_types;
  using opt2 = typename signature::output_opt2_types;
  using type = detail::type_pair<opt1, opt2>;
  // generates the appropriate `delegated<...>` type from given signatures
  using delegated_type =
    typename std::conditional<
      std::is_same<opt2, detail::empty_type_list>::value,
      typename detail::tl_apply<opt1, delegated>::type,
      delegated<either_or_t<opt1, opt2>>
    >::type;
};

template <class... Ts>
struct common_result_type;

template <class T>
struct common_result_type<T> {
  using type = T;
};

template <class T, class... Us>
struct common_result_type<T, T, Us...> {
  using type = typename common_result_type<T, Us...>::type;
};

template <class T1, class T2, class... Us>
struct common_result_type<T1, T2, Us...> {
  using type = void;
};

template <class OrigSigs, class DestSigs, class ArgTypes>
struct sender_signature_checker {
  static void check() {
    using dest_output_types =
      typename deduce_output_type<
        DestSigs, ArgTypes
      >::type;
    sender_signature_checker<
      DestSigs, OrigSigs,
      typename dest_output_types::first
    >::check();
    sender_signature_checker<
      DestSigs, OrigSigs,
      typename dest_output_types::second
    >::check();
  }
};

template <class OrigSigs, class DestSigs>
struct sender_signature_checker<OrigSigs, DestSigs, detail::type_list<void>> {
  static void check() {}
};

template <class OrigSigs, class DestSigs>
struct sender_signature_checker<OrigSigs, DestSigs, detail::type_list<>> {
  static void check() {}
};

} // namespace detail
} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_DETAIL_TYPED_ACTOR_DETAIL_HPP
