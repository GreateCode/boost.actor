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
 * Dominik Charousset <dominik.charousset@haw-hamburg.de>                     *
 *                                                                            *
 * Distributed under the Boost Software License, Version 1.0. See             *
 * accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt  *
\******************************************************************************/


#ifndef BOOST_ACTOR_FUNCTOR_BASED_ACTOR_HPP
#define BOOST_ACTOR_FUNCTOR_BASED_ACTOR_HPP

#include <type_traits>

#include "boost/actor/event_based_actor.hpp"

namespace boost {
namespace actor {
namespace detail {

class functor_based_actor : public event_based_actor {

 public:

    typedef event_based_actor* pointer;

    typedef std::function<behavior(event_based_actor*)> make_behavior_fun;

    typedef std::function<void(event_based_actor*)> void_fun;

    template<typename F, typename... Ts>
    functor_based_actor(F f, Ts&&... vs) {
        typedef typename detail::get_callable_trait<F>::type trait;
        typedef typename trait::arg_types arg_types;
        typedef typename trait::result_type result_type;
        constexpr bool returns_behavior = std::is_convertible<
                                              result_type,
                                              behavior
                                          >::value;
        constexpr bool uses_first_arg = std::is_same<
                                            typename detail::tl_head<
                                                arg_types
                                            >::type,
                                            pointer
                                        >::value;
        std::integral_constant<bool, returns_behavior> token1;
        std::integral_constant<bool, uses_first_arg>   token2;
        set(token1, token2, std::move(f), std::forward<Ts>(vs)...);
    }

    behavior make_behavior() override;

 private:

    template<typename F>
    void set(std::true_type, std::true_type, F&& fun) {
        // behavior (pointer)
        m_make_behavior = std::forward<F>(fun);
    }

    template<typename F>
    void set(std::false_type, std::true_type, F fun) {
        // void (pointer)
        m_make_behavior = [fun](pointer ptr) {
            fun(ptr);
            return behavior{};
        };
    }

    template<typename F>
    void set(std::true_type, std::false_type, F fun) {
        // behavior (void)
        m_make_behavior = [fun](pointer) { return fun(); };
    }

    template<typename F>
    void set(std::false_type, std::false_type, F fun) {
        // void (void)
        m_make_behavior = [fun](pointer) {
            fun();
            return behavior{};
        };
    }

    template<class Token, typename F, typename T0, typename... Ts>
    void set(Token t1, std::true_type t2, F fun, T0&& arg0, Ts&&... args) {
        set(t1, t2, std::bind(fun,
                              std::placeholders::_1,
                              std::forward<T0>(arg0),
                              std::forward<Ts>(args)...));
    }

    template<class Token, typename F, typename T0, typename... Ts>
    void set(Token t1, std::false_type t2, F fun, T0&& arg0,  Ts&&... args) {
        set(t1, t2, std::bind(fun,
                              std::forward<T0>(arg0),
                              std::forward<Ts>(args)...));
    }

    make_behavior_fun m_make_behavior;

};

} // namespace detail
} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_FUNCTOR_BASED_ACTOR_HPP
