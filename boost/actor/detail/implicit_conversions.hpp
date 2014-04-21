/******************************************************************************\
 *           ___        __                                                    *
 *          /\_ \    __/\ \                                                   *
 *          \//\ \  /\_\ \ \____    ___   _____   _____      __               *
 *            \ \ \ \/\ \ \ '__`\  /'___\/\ '__`\/\ '__`\  /'__`\             *
 *             \_\ \_\ \ \ \ \L\ \/\ \__/\ \ \L\ \ \ \L\ \/\ \L\.\_           *
 *             /\____\\ \_\ \_,__/\ \____\\ \ ,__/\ \ ,__/\ \__/.\_\          *
 *             \/____/ \/_/\/___/  \/____/ \ \ \/  \ \ \/  \/__/\/_/          *
 *                                          \ \_\   \ \_\                     *
 *                                           \/_/    \/_/                     *
 *                                                                            *
 * Copyright (C) 2011-2013                                                    *
 * Dominik Charousset <dominik.charousset@haw-hamburg.de>                     *
 *                                                                            *
 * This file is part of libcppa.                                              *
 * libcppa is free software: you can redistribute it and/or modify it under   *
 * the terms of the GNU Lesser General Public License as published by the     *
 * Free Software Foundation; either version 2.1 of the License,               *
 * or (at your option) any later version.                                     *
 *                                                                            *
 * libcppa is distributed in the hope that it will be useful,                 *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                       *
 * See the GNU Lesser General Public License for more details.                *
 *                                                                            *
 * You should have received a copy of the GNU Lesser General Public License   *
 * along with libcppa. If not, see <http://www.gnu.org/licenses/>.            *
\******************************************************************************/


#ifndef BOOST_ACTOR_IMPLICIT_CONVERSIONS_HPP
#define BOOST_ACTOR_IMPLICIT_CONVERSIONS_HPP

#include <string>
#include <type_traits>

#include "boost/actor/util/type_traits.hpp"

namespace boost { namespace actor { class local_actor; } }

namespace boost {
namespace actor {
namespace detail {

template<typename T>
struct implicit_conversions {

    typedef typename util::replace_type<
                T,
                std::string,
                std::is_same<T, const char*>,
                std::is_same<T, char*>,
                util::is_array_of<T, char>,
                util::is_array_of<T, const char>
            >::type
            subtype1;

    typedef typename util::replace_type<
                subtype1,
                std::u16string,
                std::is_same<subtype1, const char16_t*>,
                std::is_same<subtype1, char16_t*>,
                util::is_array_of<subtype1, char16_t>
            >::type
            subtype2;

    typedef typename util::replace_type<
                subtype2,
                std::u32string,
                std::is_same<subtype2, const char32_t*>,
                std::is_same<subtype2, char32_t*>,
                util::is_array_of<subtype2, char32_t>
            >::type
            subtype3;

    typedef typename util::replace_type<
                subtype3,
                actor,
                std::is_convertible<T, abstract_actor*>,
                std::is_same<scoped_actor, T>
            >::type
            type;

};

template<typename T>
struct strip_and_convert {
    typedef typename implicit_conversions<typename util::rm_const_and_ref<T>::type>::type
            type;
};

} // namespace detail
} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_IMPLICIT_CONVERSIONS_HPP
