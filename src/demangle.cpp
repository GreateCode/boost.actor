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


#include <stdexcept>

#include "boost/actor/config.hpp"
#include "boost/actor/detail/demangle.hpp"

#if defined(BOOST_ACTOR_GCC) || defined(BOOST_ACTOR_CLANG)
#include <cxxabi.h>
#endif

#include <stdlib.h>

namespace boost {
namespace actor {
namespace detail {

std::string demangle(const char* decorated) {
    size_t size;
    int status;
    std::unique_ptr<char, void (*)(void*)> undecorated{
        abi::__cxa_demangle(decorated, nullptr, &size, &status),
        std::free
    };
    if (status != 0) {
        std::string error_msg = "Could not demangle type name ";
        error_msg += decorated;
        throw std::logic_error(error_msg);
    }
    std::string result; // the undecorated typeid name
    result.reserve(size);
    const char* cstr = undecorated.get();
    // filter unnecessary characters from undecorated
    char c = *cstr;
    while (c != '\0') {
        if (c == ' ') {
            char previous_c = result.empty() ? ' ' : *(result.rbegin());
            // get next non-space character
            for (c = *++cstr; c == ' '; c = *++cstr) { }
            if (c != '\0') {
                // skip whitespace unless it separates two alphanumeric
                // characters (such as in "unsigned int")
                if (isalnum(c) && isalnum(previous_c)) {
                    result += ' ';
                    result += c;
                }
                else {
                    result += c;
                }
                c = *++cstr;
            }
        }
        else {
            result += c;
            c = *++cstr;
        }
    }
#   ifdef BOOST_ACTOR_CLANG
    // replace "std::__1::" with "std::" (fixes strange clang names)
    std::string needle = "std::__1::";
    std::string fixed_string = "std::";
    for (auto pos = result.find(needle); pos != std::string::npos; pos = result.find(needle)) {
        result.replace(pos, needle.size(), fixed_string);
    }
#   endif
    return result;
}

std::string demangle(const std::type_info& tinf) {
    return demangle(tinf.name());
}

} } // namespace actor
} // namespace boost::detail
