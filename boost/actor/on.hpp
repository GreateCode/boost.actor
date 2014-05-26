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
 * Copyright (C) 2011 - 2014                                                  *
 * Dominik Charousset <dominik.charousset (at) haw-hamburg.de>                *
 *                                                                            *
 * Distributed under the Boost Software License, Version 1.0. See             *
 * accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt  *
\******************************************************************************/


#ifndef BOOST_ACTOR_ON_HPP
#define BOOST_ACTOR_ON_HPP

#include <chrono>
#include <memory>
#include <functional>
#include <type_traits>

#include "boost/actor/unit.hpp"
#include "boost/actor/atom.hpp"
#include "boost/actor/anything.hpp"
#include "boost/actor/message.hpp"
#include "boost/actor/duration.hpp"
#include "boost/actor/match_expr.hpp"
#include "boost/actor/skip_message.hpp"
#include "boost/actor/may_have_timeout.hpp"
#include "boost/actor/timeout_definition.hpp"

#include "boost/actor/detail/type_list.hpp"
#include "boost/actor/detail/arg_match_t.hpp"
#include "boost/actor/detail/type_traits.hpp"

#include "boost/actor/detail/boxed.hpp"
#include "boost/actor/detail/unboxed.hpp"
#include "boost/actor/detail/implicit_conversions.hpp"

namespace boost {
namespace actor {
namespace detail {

template<bool IsFun, typename T>
struct add_ptr_to_fun_ { typedef T* type; };

template<typename T>
struct add_ptr_to_fun_<false, T> { typedef T type; };

template<typename T>
struct add_ptr_to_fun : add_ptr_to_fun_<std::is_function<T>::value, T> { };

template<bool ToVoid, typename T>
struct to_void_impl { typedef unit_t type; };

template<typename T>
struct to_void_impl<false, T> { typedef typename add_ptr_to_fun<T>::type type; };

template<typename T>
struct boxed_to_void : to_void_impl<is_boxed<T>::value, T> { };

template<typename T>
struct boxed_to_void<std::function<optional<T> (const T&)>>
        : to_void_impl<is_boxed<T>::value, T> { };

template<typename T>
struct boxed_and_callable_to_void
: to_void_impl<is_boxed<T>::value || detail::is_callable<T>::value, T> { };

class behavior_rvalue_builder {

 public:

    constexpr behavior_rvalue_builder(const duration& d) : m_tout(d) { }

    template<typename F>
    timeout_definition<F> operator>>(F&& f) const {
        return {m_tout, std::forward<F>(f)};
    }

 private:

    duration m_tout;

};

struct rvalue_builder_args_ctor { };

template<class Left, class Right>
struct disjunct_rvalue_builders {

 public:

    disjunct_rvalue_builders(Left l, Right r)
    : m_left(std::move(l)), m_right(std::move(r)) { }

    template<typename Expr>
    auto operator>>(Expr expr)
         -> decltype((*(static_cast<Left*>(nullptr)) >> expr).or_else(
                      *(static_cast<Right*>(nullptr)) >> expr)) const {
        return (m_left >> expr).or_else(m_right >> expr);
    }

 private:

    Left m_left;
    Right m_right;

};

struct tuple_maker {
    template<typename... Ts>
    inline auto operator()(Ts&&... args)
    -> decltype(std::make_tuple(std::forward<Ts>(args)...)) {
        return std::make_tuple(std::forward<Ts>(args)...);
    }
};

template<class Transformers, class Pattern>
struct rvalue_builder {

    typedef typename detail::tl_back<Pattern>::type back_type;

    static constexpr bool is_complete =
            !std::is_same<detail::arg_match_t, back_type>::value;

    typedef typename detail::tl_apply<Transformers, std::tuple>::type fun_container;

    fun_container m_funs;

 public:

    rvalue_builder() = default;

    template<typename... Ts>
    rvalue_builder(rvalue_builder_args_ctor, const Ts&... args) : m_funs(args...) { }

    rvalue_builder(fun_container arg1) : m_funs(std::move(arg1)) { }

    template<typename Expr>
    match_expr<typename get_case<is_complete, Expr, Transformers, Pattern>::type>
    operator>>(Expr expr) const {
        typedef typename get_case<
                    is_complete,
                    Expr,
                    Transformers,
                    Pattern
                >::type
                lifted_expr;
        // adjust m_funs to exactly match expected projections in match case
        typedef typename lifted_expr::projections_list target;
        typedef typename tl_trim<Transformers>::type trimmed_projections;
        tuple_maker f;
        auto lhs = apply_args(f, get_indices(trimmed_projections{}), m_funs);
        typename tl_apply<
                typename tl_slice<
                    target,
                    tl_size<trimmed_projections>::value,
                    tl_size<target>::value
                >::type,
                std::tuple
            >::type
            rhs;
        // done
        return lifted_expr{std::move(expr), std::tuple_cat(lhs, rhs)};
    }

    template<class T, class P>
    disjunct_rvalue_builders<rvalue_builder, rvalue_builder<T, P> >
    operator||(rvalue_builder<T, P> other) const {
        return {*this, std::move(other)};
    }

};

template<bool IsCallable, typename T>
struct pattern_type_ {
    typedef detail::get_callable_trait<T> ctrait;
    typedef typename ctrait::arg_types args;
    static_assert(detail::tl_size<args>::value == 1, "only unary functions allowed");
    typedef typename detail::rm_const_and_ref<typename detail::tl_head<args>::type>::type type;
};

template<typename T>
struct pattern_type_<false, T> {
    typedef typename implicit_conversions<
                typename detail::rm_const_and_ref<
                    typename detail::unboxed<T>::type
                >::type
            >::type
            type;
};

template<typename T>
struct pattern_type : pattern_type_<detail::is_callable<T>::value && !detail::is_boxed<T>::value, T> {
};

} // namespace detail
} // namespace actor
} // namespace boost

namespace boost {
namespace actor {

/**
 * @brief A wildcard that matches any number of any values.
 */
constexpr anything any_vals = anything{};

#ifdef BOOST_ACTOR_DOCUMENTATION

/**
 * @brief A wildcard that matches the argument types
 *        of a given callback. Must be the last argument to {@link on()}.
 * @see {@link math_actor_example.cpp Math Actor Example}
 */
constexpr __unspecified__ arg_match;

/**
 * @brief Left-hand side of a partial function expression.
 *
 * Equal to <tt>on(arg_match)</tt>.
 */
constexpr __unspecified__ on_arg_match;

/**
 * @brief A wildcard that matches any value of type @p T.
 * @see {@link math_actor_example.cpp Math Actor Example}
 */
template<typename T>
__unspecified__ val();

/**
 * @brief Left-hand side of a partial function expression that matches values.
 *
 * This overload can be used with the wildcards {@link cppa::val val},
 * {@link cppa::any_vals any_vals} and {@link cppa::arg_match arg_match}.
 */
template<typename T, typename... Ts>
__unspecified__ on(const T& arg, const Ts&... args);

/**
 * @brief Left-hand side of a partial function expression that matches types.
 *
 * This overload matches types only. The type {@link cppa::anything anything}
 * can be used as wildcard to match any number of elements of any types.
 */
template<typename... Ts>
__unspecified__ on();

/**
 * @brief Left-hand side of a partial function expression that matches types.
 *
 * This overload matches up to four leading atoms.
 * The type {@link cppa::anything anything}
 * can be used as wildcard to match any number of elements of any types.
 */
template<atom_value... Atoms, typename... Ts>
__unspecified__ on();

/**
 * @brief Converts @p arg to a match expression by returning
 *        <tt>on_arg_match >> arg</tt> if @p arg is a callable type,
 *        otherwise returns @p arg.
 */
template<typename T>
__unspecified__ lift_to_match_expr(T arg);

#else

template<typename T>
constexpr typename detail::boxed<T>::type val() {
    return typename detail::boxed<T>::type();
}

typedef typename detail::boxed<detail::arg_match_t>::type boxed_arg_match_t;

constexpr boxed_arg_match_t arg_match = boxed_arg_match_t();

template<typename T, typename Predicate>
std::function<optional<T> (const T&)> guarded(Predicate p, T value) {
    return [=](const T& other) -> optional<T> {
        if (p(other, value)) return value;
        return none;
    };
}

// special case covering arg_match as argument to guarded()
template<typename T, typename Predicate>
unit_t guarded(Predicate, const detail::wrapped<T>&) {
    return {};
}

template<typename T, bool Callable = detail::is_callable<T>::value>
struct to_guard {
    static std::function<optional<T> (const T&)> _(const T& value) {
        return guarded<T>(std::equal_to<T>{}, value);
    }
};

template<>
struct to_guard<anything, false> {
    template<typename U>
    static unit_t _(const U&) {
        return {};
    }
};

template<typename T>
struct to_guard<detail::wrapped<T>, false> : to_guard<anything> { };

template<typename T>
struct is_optional : std::false_type { };

template<typename T>
struct is_optional<optional<T>> : std::true_type { };

template<typename T>
struct to_guard<T, true> {
    typedef typename detail::get_callable_trait<T>::type ct;
    typedef typename ct::arg_types arg_types;
    static_assert(detail::tl_size<arg_types>::value == 1,
                  "projection/guard must take exactly one argument");
    static_assert(is_optional<typename ct::result_type>::value,
                  "projection/guard must return an optional value");
    typedef typename std::conditional<std::is_function<T>::value, T*, T>::type
            value_type;
    static value_type _(value_type val) { return val; }
};

template<typename T>
struct to_guard<detail::wrapped<T>(), true> : to_guard<anything> { };


template<typename T, typename... Ts>
auto on(const T& arg, const Ts&... args)
->  detail::rvalue_builder<
        detail::type_list<
            decltype(to_guard<typename detail::strip_and_convert<T>::type>::_(arg)),
            decltype(to_guard<typename detail::strip_and_convert<Ts>::type>::_(args))...
        >,
        detail::type_list<typename detail::pattern_type<T>::type,
                          typename detail::pattern_type<Ts>::type...>
    > {
    return {detail::rvalue_builder_args_ctor{},
            to_guard<typename detail::strip_and_convert<T>::type>::_(arg),
            to_guard<typename detail::strip_and_convert<Ts>::type>::_(args)...};
}

inline detail::rvalue_builder<detail::empty_type_list,
                              detail::empty_type_list     > on() {
    return {};
}

template<typename T0, typename... Ts>
detail::rvalue_builder<detail::empty_type_list,
                       detail::type_list<T0, Ts...> >
on() {
    return {};
}

template<atom_value A0, typename... Ts>
decltype(on(A0, val<Ts>()...)) on() {
    return on(A0, val<Ts>()...);
}

template<atom_value A0, atom_value A1, typename... Ts>
decltype(on(A0, A1, val<Ts>()...)) on() {
    return on(A0, A1, val<Ts>()...);
}

template<atom_value A0, atom_value A1, atom_value A2, typename... Ts>
decltype(on(A0, A1, A2, val<Ts>()...)) on() {
    return on(A0, A1, A2, val<Ts>()...);
}

template<atom_value A0, atom_value A1, atom_value A2, atom_value A3,
         typename... Ts>
decltype(on(A0, A1, A2, A3, val<Ts>()...)) on() {
    return on(A0, A1, A2, A3, val<Ts>()...);
}

template<class Rep, class Period>
constexpr detail::behavior_rvalue_builder
after(const std::chrono::duration<Rep, Period>& d) {
    return { duration(d) };
}

inline decltype(on<anything>()) others() {
    return on<anything>();
}

// some more convenience

namespace detail {

class on_the_fly_rvalue_builder {

 public:

    constexpr on_the_fly_rvalue_builder() { }

    template<typename Expr>
    match_expr<
        typename get_case<
            false,
            Expr,
            detail::empty_type_list,
            detail::empty_type_list
        >::type>
    operator>>(Expr expr) const {
        typedef typename get_case<
                    false,
                    Expr,
                    detail::empty_type_list,
                    detail::empty_type_list
                >::type
                result_type;
        return result_type{std::move(expr)};
    }

};

} // namespace detail

constexpr detail::on_the_fly_rvalue_builder on_arg_match;

#endif // BOOST_ACTOR_DOCUMENTATION

} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_ON_HPP
