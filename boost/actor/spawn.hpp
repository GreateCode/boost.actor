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


#ifndef BOOST_ACTOR_SPAWN_HPP
#define BOOST_ACTOR_SPAWN_HPP

#include <type_traits>

#include "boost/actor/policy.hpp"
#include "boost/actor/scheduler.hpp"
#include "boost/actor/spawn_fwd.hpp"
#include "boost/actor/typed_actor.hpp"
#include "boost/actor/spawn_options.hpp"
#include "boost/actor/typed_event_based_actor.hpp"

#include "boost/actor/detail/logging.hpp"
#include "boost/actor/detail/cs_thread.hpp"
#include "boost/actor/detail/type_traits.hpp"
#include "boost/actor/detail/make_counted.hpp"
#include "boost/actor/detail/proper_actor.hpp"
#include "boost/actor/detail/typed_actor_util.hpp"
#include "boost/actor/detail/functor_based_actor.hpp"
#include "boost/actor/detail/implicit_conversions.hpp"
#include "boost/actor/detail/functor_based_blocking_actor.hpp"

namespace boost {
namespace actor {

class execution_unit;

namespace detail {

template<class C, spawn_options Os, typename BeforeLaunch, typename... Ts>
intrusive_ptr<C> spawn_impl(execution_unit* host,
                            BeforeLaunch before_launch_fun,
                            Ts&&... args) {
    static_assert(!std::is_base_of<blocking_actor, C>::value ||
                  has_blocking_api_flag(Os),
                  "C is derived type of blocking_actor but "
                  "blocking_api_flag is missing");
    static_assert(is_unbound(Os),
                  "top-level spawns cannot have monitor or link flag");
    BOOST_ACTOR_LOGF_TRACE("spawn " << detail::demangle<C>());
    // runtime check wheter context_switching_resume can be used,
    // i.e., add the detached flag if libcppa was compiled
    // without cs_thread support when using the blocking API
    if (has_blocking_api_flag(Os)
            && !has_detach_flag(Os)
            && detail::cs_thread::is_disabled_feature) {
        return spawn_impl<C, Os + detached>(host, before_launch_fun,
                                            std::forward<Ts>(args)...);
    }
    using scheduling_policy = typename std::conditional<
                                  has_detach_flag(Os),
                                  policy::no_scheduling,
                                  policy::cooperative_scheduling
                              >::type;
    using priority_policy = typename std::conditional<
                                has_priority_aware_flag(Os),
                                policy::prioritizing,
                                policy::not_prioritizing
                            >::type;
    using resume_policy = typename std::conditional<
                              has_blocking_api_flag(Os),
                              typename std::conditional<
                                  has_detach_flag(Os),
                                  policy::no_resume,
                                  policy::context_switching_resume
                              >::type,
                              policy::event_based_resume
                          >::type;
    using invoke_policy = typename std::conditional<
                              has_blocking_api_flag(Os),
                              policy::nestable_invoke,
                              policy::sequential_invoke
                          >::type;
    using policies = policy::policies<scheduling_policy,
                                      priority_policy,
                                      resume_policy,
                                      invoke_policy>;
    using proper_impl = detail::proper_actor<C, policies>;
    auto ptr = detail::make_counted<proper_impl>(std::forward<Ts>(args)...);
    BOOST_ACTOR_LOGF_DEBUG("spawned actor with ID " << ptr->id());
    BOOST_ACTOR_PUSH_AID(ptr->id());
    before_launch_fun(ptr.get());
    ptr->launch(has_hide_flag(Os), host);
    return ptr;
}

template<typename T>
struct spawn_fwd {
    static inline T& fwd(T& arg) { return arg; }
    static inline const T& fwd(const T& arg) { return arg; }
    static inline T&& fwd(T&& arg) { return std::move(arg); }
};

template<typename T, typename... Ts>
struct spawn_fwd<T (Ts...)> {
    typedef T (*pointer) (Ts...);
    static inline pointer fwd(pointer arg) { return arg; }
};

template<>
struct spawn_fwd<scoped_actor> {
    template<typename T>
    static inline actor fwd(T& arg) { return arg; }
};

} // namespace detail

// forwards the arguments to spawn_impl, replacing pointers
// to actors with instances of 'actor'
template<class C, spawn_options Os, typename BeforeLaunch, typename... Ts>
intrusive_ptr<C> spawn_class(execution_unit* host,
                             BeforeLaunch before_launch_fun,
                             Ts&&... args) {
    return detail::spawn_impl<C, Os>(host,
                                     before_launch_fun,
            detail::spawn_fwd<typename detail::rm_const_and_ref<Ts>::type>::fwd(
                    std::forward<Ts>(args))...);
}

template<spawn_options Os, typename BeforeLaunch, typename F, typename... Ts>
actor spawn_functor(execution_unit* eu,
                    BeforeLaunch cb,
                    F fun,
                    Ts&&... args) {
    typedef typename detail::get_callable_trait<F>::type trait;
    typedef typename trait::arg_types arg_types;
    typedef typename detail::tl_head<arg_types>::type first_arg;
    constexpr bool is_blocking = has_blocking_api_flag(Os);
    constexpr bool has_ptr_arg = std::is_pointer<first_arg>::value;
    constexpr bool has_blocking_self = std::is_same<
                                                  first_arg,
                                                  blocking_actor*
                                              >::value;
    constexpr bool has_nonblocking_self = std::is_same<
                                                     first_arg,
                                                     event_based_actor*
                                                 >::value;
    static_assert(!is_blocking || has_blocking_self || !has_ptr_arg,
                  "functor-based actors with blocking_actor* as first "
                  "argument need to be spawned using the blocking_api flag");
    static_assert(is_blocking || has_nonblocking_self || !has_ptr_arg,
                  "functor-based actors with event_based_actor* as first "
                  "argument cannot be spawned using the blocking_api flag");
    using base_class = typename std::conditional<
                           is_blocking,
                           detail::functor_based_blocking_actor,
                           detail::functor_based_actor
                       >::type;
    return spawn_class<base_class, Os>(eu, cb, fun, std::forward<Ts>(args)...);
}

/**
 * @ingroup ActorCreation
 * @{
 */

/**
 * @brief Spawns an actor of type @p C.
 * @param args Constructor arguments.
 * @tparam Impl Subtype of {@link event_based_actor} or {@link sb_actor}.
 * @tparam Os Optional flags to modify <tt>spawn</tt>'s behavior.
 * @returns An {@link actor} to the spawned {@link actor}.
 */
template<class Impl, spawn_options Os = no_spawn_options, typename... Ts>
actor spawn(Ts&&... args) {
    return spawn_class<Impl, Os>(nullptr, empty_before_launch_callback{},
                                 std::forward<Ts>(args)...);
}

/**
 * @brief Spawns a new {@link actor} that evaluates given arguments.
 * @param args A functor followed by its arguments.
 * @tparam Os Optional flags to modify <tt>spawn</tt>'s behavior.
 * @returns An {@link actor} to the spawned {@link actor}.
 */
template<spawn_options Os = no_spawn_options, typename... Ts>
actor spawn(Ts&&... args) {
    static_assert(sizeof...(Ts) > 0, "too few arguments provided");
    return spawn_functor<Os>(nullptr, empty_before_launch_callback{},
                             std::forward<Ts>(args)...);
}

/**
 * @brief Spawns an actor of type @p C that immediately joins @p grp.
 * @param args Constructor arguments.
 * @tparam Impl Subtype of {@link event_based_actor} or {@link sb_actor}.
 * @tparam Os Optional flags to modify <tt>spawn</tt>'s behavior.
 * @returns An {@link actor} to the spawned {@link actor}.
 * @note The spawned has joined the group before this function returns.
 */
template<class Impl, spawn_options Os = no_spawn_options, typename... Ts>
actor spawn_in_group(const group& grp, Ts&&... args) {
    return spawn_class<Impl, Os>(nullptr, group_subscriber{grp},
                                 std::forward<Ts>(args)...);
}

/**
 * @brief Spawns a new actor that evaluates given arguments and
 *        immediately joins @p grp.
 * @param args A functor followed by its arguments.
 * @tparam Os Optional flags to modify <tt>spawn</tt>'s behavior.
 * @returns An {@link actor} to the spawned {@link actor}.
 * @note The spawned has joined the group before this function returns.
 */
template<spawn_options Os = no_spawn_options, typename... Ts>
actor spawn_in_group(const group& grp, Ts&&... args) {
    static_assert(sizeof...(Ts) > 0, "too few arguments provided");
    return spawn_functor<Os>(nullptr, group_subscriber{grp},
                             std::forward<Ts>(args)...);
}

namespace detail {

template<typename... Rs>
class functor_based_typed_actor : public typed_event_based_actor<Rs...> {

    typedef typed_event_based_actor<Rs...> super;

 public:

    typedef typed_event_based_actor<Rs...>* pointer;
    typedef typename super::behavior_type   behavior_type;

    typedef std::function<behavior_type ()>        no_arg_fun;
    typedef std::function<behavior_type (pointer)> one_arg_fun1;
    typedef std::function<void (pointer)>          one_arg_fun2;

    template<typename F, typename... Ts>
    functor_based_typed_actor(F fun, Ts&&... args) {
        typedef typename detail::get_callable_trait<F>::type trait;
        typedef typename trait::arg_types arg_types;
        typedef typename trait::result_type result_type;
        constexpr bool returns_behavior = std::is_same<
                                              result_type,
                                              behavior_type
                                          >::value;
        constexpr bool uses_first_arg = std::is_same<
                                            typename detail::tl_head<
                                                arg_types
                                            >::type,
                                            pointer
                                        >::value;
        std::integral_constant<bool, returns_behavior> token1;
        std::integral_constant<bool, uses_first_arg>   token2;
        set(token1, token2, std::move(fun), std::forward<Ts>(args)...);
    }

 protected:

    behavior_type make_behavior() override {
        return m_fun(this);
    }

 private:

    template<typename F>
    void set(std::true_type, std::true_type, F&& fun) {
        // behavior_type (pointer)
        m_fun = std::forward<F>(fun);
    }

    template<typename F>
    void set(std::false_type, std::true_type, F fun) {
        // void (pointer)
        m_fun = [fun](pointer ptr) {
            fun(ptr);
            return behavior_type{};
        };
    }

    template<typename F>
    void set(std::true_type, std::false_type, F fun) {
        // behavior_type ()
        m_fun = [fun](pointer) { return fun(); };
    }

    // (false_type, false_type) is an invalid functor for typed actors

    template<class Token, typename F, typename T0, typename... Ts>
    void set(Token t1, std::true_type t2, F fun, T0&& arg0, Ts&&... args) {
        set(t1, t2, std::bind(fun, std::placeholders::_1,
                              std::forward<T0>(arg0),
                              std::forward<Ts>(args)...));
    }

    template<class Token, typename F, typename T0, typename... Ts>
    void set(Token t1, std::false_type t2, F fun, T0&& arg0,  Ts&&... args) {
        set(t1, t2, std::bind(fun, std::forward<T0>(arg0),
                              std::forward<Ts>(args)...));
    }

    one_arg_fun1 m_fun;

};

template<class TypedBehavior, class FirstArg>
struct infer_typed_actor_base;

template<typename... Rs, class FirstArg>
struct infer_typed_actor_base<typed_behavior<Rs...>, FirstArg> {
    typedef functor_based_typed_actor<Rs...> type;
};

template<typename... Rs>
struct infer_typed_actor_base<void, typed_event_based_actor<Rs...>*> {
    typedef functor_based_typed_actor<Rs...> type;
};

} // namespace detail

/**
 * @brief Spawns a typed actor of type @p C.
 * @param args Constructor arguments.
 * @tparam C Subtype of {@link typed_event_based_actor}.
 * @tparam Os Optional flags to modify <tt>spawn</tt>'s behavior.
 * @returns A {@link typed_actor} handle to the spawned actor.
 */
template<class C, spawn_options Os = no_spawn_options, typename... Ts>
typename detail::actor_handle_from_signature_list<
    typename C::signatures
>::type
spawn_typed(Ts&&... args) {
    return spawn_class<C, Os>(nullptr, empty_before_launch_callback{},
                              std::forward<Ts>(args)...);
}

template<spawn_options Os, typename BeforeLaunch, typename F, typename... Ts>
typename detail::infer_typed_actor_handle<
    typename detail::get_callable_trait<F>::result_type,
    typename detail::tl_head<
        typename detail::get_callable_trait<F>::arg_types
    >::type
>::type
spawn_typed_functor(execution_unit* eu, BeforeLaunch bl, F fun, Ts&&... args) {
    typedef typename detail::infer_typed_actor_base<
                typename detail::get_callable_trait<F>::result_type,
                typename detail::tl_head<
                    typename detail::get_callable_trait<F>::arg_types
                >::type
            >::type
            impl;
    return spawn_class<impl, Os>(eu, bl, fun, std::forward<Ts>(args)...);
}

/**
 * @brief Spawns a typed actor from a functor.
 * @param args A functor followed by its arguments.
 * @tparam Os Optional flags to modify <tt>spawn</tt>'s behavior.
 * @returns An {@link actor} to the spawned {@link actor}.
 */
template<spawn_options Os = no_spawn_options, typename F, typename... Ts>
typename detail::infer_typed_actor_handle<
    typename detail::get_callable_trait<F>::result_type,
    typename detail::tl_head<
        typename detail::get_callable_trait<F>::arg_types
    >::type
>::type
spawn_typed(F fun, Ts&&... args) {
    return spawn_typed_functor<Os>(nullptr, empty_before_launch_callback{},
                                   std::move(fun), std::forward<Ts>(args)...);
}

/** @} */

} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_SPAWN_HPP
