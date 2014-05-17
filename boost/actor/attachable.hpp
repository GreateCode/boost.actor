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


#ifndef BOOST_ACTOR_ATTACHABLE_HPP
#define BOOST_ACTOR_ATTACHABLE_HPP

#include <memory>
#include <cstdint>
#include <typeinfo>

namespace boost {
namespace actor {

/**
 * @brief Callback utility class.
 */
class attachable {

    attachable(const attachable&) = delete;
    attachable& operator=(const attachable&) = delete;

 protected:

    attachable() = default;

 public:

    /**
     * @brief Represents a pointer to a value with its RTTI.
     */
    struct token {

        /**
         * @brief Denotes the type of @c ptr.
         */
        const std::type_info& subtype;

        /**
         * @brief Any value, used to identify @c attachable instances.
         */
        const void* ptr;

        token(const std::type_info& subtype, const void* ptr);

    };

    virtual ~attachable();

    /**
     * @brief Executed if the actor finished execution with given @p reason.
     *
     * The default implementation does nothing.
     * @param reason The exit rason of the observed actor.
     */
    virtual void actor_exited(std::uint32_t reason) = 0;

    /**
     * @brief Selects a group of @c attachable instances by @p what.
     * @param what A value that selects zero or more @c attachable instances.
     * @returns @c true if @p what selects this instance; otherwise @c false.
     */
    virtual bool matches(const token& what) = 0;

};

/**
 * @brief A managed {@link attachable} pointer.
 * @relates attachable
 */
typedef std::unique_ptr<attachable> attachable_ptr;

} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_ATTACHABLE_HPP
