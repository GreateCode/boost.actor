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


#include <sstream>

#include "boost/actor/duration.hpp"

namespace boost {
namespace actor {

namespace {

inline std::uint64_t ui64_val(const duration& d) {
    return static_cast<std::uint64_t>(d.unit) * d.count;
}

} // namespace <anonmyous>

bool operator==(const duration& lhs, const duration& rhs) {
    return (lhs.unit == rhs.unit ? lhs.count == rhs.count
                                 : ui64_val(lhs) == ui64_val(rhs));
}

std::string duration::to_string() const {
    if (unit == time_unit::invalid) return "-invalid-";
    std::ostringstream oss;
    oss << count;
    switch (unit) {
        case time_unit::invalid: oss << "?"; break;
        case time_unit::seconds: oss << "s"; break;
        case time_unit::milliseconds: oss << "ms"; break;
        case time_unit::microseconds: oss << "us"; break;
    }
    return oss.str();
}

} // namespace actor
} // namespace boost
