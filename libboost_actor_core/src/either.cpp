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
 * Distributed under the terms and conditions of the BSD 3-Clause License or  *
 * (at your option) under the terms and conditions of the Boost Software      *
 * License 1.0. See accompanying files LICENSE and LICENSE_ALTERNATIVE.       *
 *                                                                            *
 * If you did not receive a copy of the license files, see                    *
 * http://opensource.org/licenses/BSD-3-Clause and                            *
 * http://www.boost.org/LICENSE_1_0.txt.                                      *
 ******************************************************************************/

#include "boost/actor/either.hpp"

#include "boost/actor/to_string.hpp"
#include <boost/algorithm/string.hpp>

namespace boost {
namespace actor {

std::string either_or_else_type_name(size_t lefts_size,
                                     const std::string* lefts,
                                     size_t rights_size,
                                     const std::string* rights) {
  using irange = iterator_range<const std::string*>;
  std::string glue = ",";
  std::string result;
  result = "boost::actor::either<";
  result += join(irange{lefts, lefts + lefts_size}, glue);
  result += ">::or_else<";
  result += join(irange{rights, rights + rights_size}, glue);
  result += ">";
  return result;
}

} // namespace actor
} // namespace boost
