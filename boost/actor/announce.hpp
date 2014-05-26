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


#ifndef BOOST_ACTOR_ANNOUNCE_HPP
#define BOOST_ACTOR_ANNOUNCE_HPP

#include <memory>
#include <typeinfo>

#include "boost/algorithm/string/join.hpp"

#include "boost/actor/config.hpp"
#include "boost/actor/uniform_type_info.hpp"

#include "boost/actor/detail/abstract_uniform_type_info.hpp"

#include "boost/actor/detail/safe_equal.hpp"
#include "boost/actor/detail/default_uniform_type_info.hpp"

namespace boost {
namespace actor {

/**
 * @addtogroup TypeSystem
 * @{
 */

/**
 * @brief A simple example for @c announce with public accessible members.
 *
 * The output of this example program is:
 *
 * <tt>
 * foo(1, 2)<br>
 * foo_pair(3, 4)
 * </tt>
 * @example announce_1.cpp
 */

/**
 * @brief An example for @c announce with getter and setter member functions.
 *
 * The output of this example program is:
 *
 * <tt>foo(1, 2)</tt>
 * @example announce_2.cpp
 */

/**
 * @brief An example for @c announce with overloaded
 *        getter and setter member functions.
 *
 * The output of this example program is:
 *
 * <tt>foo(1, 2)</tt>
 * @example announce_3.cpp
 */

/**
 * @brief An example for @c announce with non-primitive members.
 *
 * The output of this example program is:
 *
 * <tt>bar(foo(1, 2), 3)</tt>
 * @example announce_4.cpp
 */

/**
 * @brief An advanced example for @c announce implementing serialization
 *        for a user-defined tree data type.
 * @example announce_5.cpp
 */

/**
 * @brief Adds a new type mapping to the libcppa type system.
 * @param tinfo C++ RTTI for the new type
 * @param utype Corresponding {@link uniform_type_info} instance.
 * @returns @c true if @p uniform_type was added as known
 *         instance (mapped to @p plain_type); otherwise @c false
 *         is returned and @p uniform_type was deleted.
 */
const uniform_type_info* announce(const std::type_info& tinfo,
                                  uniform_type_info_ptr utype);

// deals with member pointer
/**
 * @brief Creates meta informations for a non-trivial member @p C.
 * @param c_ptr Pointer to the non-trivial member.
 * @param args "Sub-members" of @p c_ptr
 * @see {@link announce_4.cpp announce example 4}
 * @returns A pair of @p c_ptr and the created meta informations.
 */
template<class C, class Parent, typename... Ts>
std::pair<C Parent::*, detail::abstract_uniform_type_info<C>*>
compound_member(C Parent::*c_ptr, const Ts&... args) {
    return {c_ptr, new detail::default_uniform_type_info<C>(args...)};
}

// deals with getter returning a mutable reference
/**
 * @brief Creates meta informations for a non-trivial member accessed
 *        via a getter returning a mutable reference.
 * @param getter Member function pointer to the getter.
 * @param args "Sub-members" of @p c_ptr
 * @see {@link announce_4.cpp announce example 4}
 * @returns A pair of @p c_ptr and the created meta informations.
 */
template<class C, class Parent, typename... Ts>
std::pair<C& (Parent::*)(), detail::abstract_uniform_type_info<C>*>
compound_member(C& (Parent::*getter)(), const Ts&... args) {
    return {getter, new detail::default_uniform_type_info<C>(args...)};
}

// deals with getter/setter pair
/**
 * @brief Creates meta informations for a non-trivial member accessed
 *        via a getter/setter pair.
 * @param gspair A pair of two member function pointers representing
 *               getter and setter.
 * @param args "Sub-members" of @p c_ptr
 * @see {@link announce_4.cpp announce example 4}
 * @returns A pair of @p c_ptr and the created meta informations.
 */
template<class Parent, typename GRes,
         typename SRes, typename SArg, typename... Ts>
std::pair<std::pair<GRes (Parent::*)() const, SRes (Parent::*)(SArg)>,
          detail::abstract_uniform_type_info<typename detail::rm_const_and_ref<GRes>::type>*>
compound_member(const std::pair<GRes (Parent::*)() const,
                                SRes (Parent::*)(SArg)  >& gspair,
                const Ts&... args) {
    typedef typename detail::rm_const_and_ref<GRes>::type mtype;
    return {gspair, new detail::default_uniform_type_info<mtype>(args...)};
}

/**
 * @brief Adds a new type mapping for @p C to the libcppa type system.
 * @tparam C A class that is either empty or is default constructible,
 *           copy constructible, and comparable.
 * @param args Members of @p C.
 * @warning @p announce is <b>not</b> thead safe!
 */
template<class C, typename... Ts>
inline const uniform_type_info* announce(const Ts&... args) {
    auto ptr = new detail::default_uniform_type_info<C>(args...);
    return announce(typeid(C), uniform_type_info_ptr{ptr});
}

/**
 * @}
 */

} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_ANNOUNCE_HPP
