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


#ifndef BOOST_ACTOR_SPAWN_OPTIONS_HPP
#define BOOST_ACTOR_SPAWN_OPTIONS_HPP

namespace boost {
namespace actor {

/**
 * @ingroup ActorCreation
 * @{
 */

/**
 * @brief Stores options passed to the @p spawn function family.
 */
#ifdef BOOST_ACTOR_DOCUMENTATION
class spawn_options { };
#else
enum class spawn_options : int {
    no_flags            = 0x00,
    link_flag           = 0x01,
    monitor_flag        = 0x02,
    detach_flag         = 0x04,
    hide_flag           = 0x08,
    blocking_api_flag   = 0x10,
    priority_aware_flag = 0x20
};
#endif

/**
 * @brief Concatenates two {@link spawn_options}.
 * @relates spawn_options
 */
constexpr spawn_options operator+(const spawn_options& lhs,
                                  const spawn_options& rhs) {
    return static_cast<spawn_options>( static_cast<int>(lhs)
                                     | static_cast<int>(rhs));
}

/**
 * @brief Denotes default settings.
 */
constexpr spawn_options no_spawn_options = spawn_options::no_flags;

/**
 * @brief Causes @p spawn to call <tt>self->monitor(...)</tt> immediately
 *        after the new actor was spawned.
 */
constexpr spawn_options monitored        = spawn_options::monitor_flag;

/**
 * @brief Causes @p spawn to call <tt>self->link_to(...)</tt> immediately
 *        after the new actor was spawned.
 */
constexpr spawn_options linked           = spawn_options::link_flag;

/**
 * @brief Causes the new actor to opt out of the cooperative scheduling.
 */
constexpr spawn_options detached         = spawn_options::detach_flag;

/**
 * @brief Causes the runtime to ignore the new actor in
 *        {@link await_all_actors_done()}.
 */
constexpr spawn_options hidden           = spawn_options::hide_flag;

/**
 * @brief Causes the new actor to opt in to the blocking API,
 *        i.e., the actor uses a context-switching or thread-based backend
 *        instead of the default event-based implementation.
 */
constexpr spawn_options blocking_api     = spawn_options::blocking_api_flag;

/**
 * @brief Causes the new actor to evaluate message priorities.
 * @note This implicitly causes the actor to run in its own thread.
 */
constexpr spawn_options priority_aware   = spawn_options::priority_aware_flag;

/**
 * @brief Checks wheter @p haystack contains @p needle.
 * @relates spawn_options
 */
constexpr bool has_spawn_option(spawn_options haystack, spawn_options needle) {
    return (static_cast<int>(haystack) & static_cast<int>(needle)) != 0;
}

/**
 * @brief Checks wheter the {@link detached} flag is set in @p opts.
 * @relates spawn_options
 */
constexpr bool has_detach_flag(spawn_options opts) {
    return has_spawn_option(opts, detached);
}

/**
 * @brief Checks wheter the {@link priority_aware} flag is set in @p opts.
 * @relates spawn_options
 */
constexpr bool has_priority_aware_flag(spawn_options opts) {
    return has_spawn_option(opts, priority_aware);
}

/**
 * @brief Checks wheter the {@link hidden} flag is set in @p opts.
 * @relates spawn_options
 */
constexpr bool has_hide_flag(spawn_options opts) {
    return has_spawn_option(opts, hidden);
}

/**
 * @brief Checks wheter the {@link linked} flag is set in @p opts.
 * @relates spawn_options
 */
constexpr bool has_link_flag(spawn_options opts) {
    return has_spawn_option(opts, linked);
}

/**
 * @brief Checks wheter the {@link monitored} flag is set in @p opts.
 * @relates spawn_options
 */
constexpr bool has_monitor_flag(spawn_options opts) {
    return has_spawn_option(opts, monitored);
}

/**
 * @brief Checks wheter the {@link blocking_api} flag is set in @p opts.
 * @relates spawn_options
 */
constexpr bool has_blocking_api_flag(spawn_options opts) {
    return has_spawn_option(opts, blocking_api);
}

/** @} */

/** @cond PRIVATE */

constexpr bool is_unbound(spawn_options opts) {
    return !has_monitor_flag(opts) && !has_link_flag(opts);
}

constexpr spawn_options make_unbound(spawn_options opts) {
    return static_cast<spawn_options>(
                (static_cast<int>(opts)
                 & ~(static_cast<int>(linked) | static_cast<int>(monitored))));
}

/** @endcond */

} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_SPAWN_OPTIONS_HPP
