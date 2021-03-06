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
 * Copyright (C) 2011 - 2015                                                  *
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

#include "boost/actor/replies_to.hpp"

#include "boost/actor/to_string.hpp"
#include <boost/algorithm/string.hpp>

namespace boost {
namespace actor {

std::string replies_to_type_name(size_t input_size,
                                 const std::string* input,
                                 size_t output_opt1_size,
                                 const std::string* output_opt1,
                                 size_t output_opt2_size,
                                 const std::string* output_opt2) {
  using irange = iterator_range<const std::string*>;
  std::string glue = ",";
  std::string result;
  // 'void' is not an announced type, hence we check whether uniform_typeid
  // did return a valid pointer to identify 'void' (this has the
  // possibility of false positives, but those will be catched anyways)
  result = "boost::actor::replies_to<";
  result += join(irange{input, input + input_size}, glue);
  if (output_opt2_size == 0) {
    result += ">::with<";
    result += join(irange{output_opt1, output_opt1 + output_opt1_size}, glue);
  } else {
    result += ">::with_either<";
    result += join(irange{output_opt1, output_opt1 + output_opt1_size}, glue);
    result += ">::or_else<";
    result += join(irange{output_opt2, output_opt2 + output_opt2_size}, glue);
  }
  result += ">";
  return result;
}

} // namespace actor
} // namespace boost
