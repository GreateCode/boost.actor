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

#include "boost/actor/detail/message_data.hpp"

#include <cstring>

namespace boost {
namespace actor {
namespace detail {

message_data::~message_data() {
  // nop
}

bool message_data::equals(const message_data& other) const {
  if (this == &other) {
    return true;
  }
  auto n = size();
  if (n != other.size()) {
    return false;
  }
  // step 1, get and compare type names
  std::vector<const char*> type_names;
  for (size_t i = 0; i < n; ++i) {
    auto lhs = uniform_name_at(i);
    auto rhs = other.uniform_name_at(i);
    if (lhs != rhs && strcmp(lhs, rhs) != 0) {
      return false; // type mismatch
    }
    type_names.push_back(lhs);
  }
  // step 2: compare each value individually
  for (size_t i = 0; i < n; ++i) {
    auto uti = uniform_type_info::from(type_names[i]);
    if (! uti->equals(at(i), other.at(i))) {
      return false;
    }
  }
  return true;
}

std::string message_data::tuple_type_names() const {
  std::string result = "@<>";
  for (size_t i = 0; i < size(); ++i) {
    result += "+";
    result += uniform_name_at(i);
  }
  return result;
}

message_data* message_data::cow_ptr::get_unshared() {
  auto p = ptr_.get();
  if (! p->unique()) {
    auto cptr = p->copy();
    ptr_.swap(cptr.ptr_);
    return ptr_.get();
  }
  return p;
}

} // namespace detail
} // namespace actor
} // namespace boost
