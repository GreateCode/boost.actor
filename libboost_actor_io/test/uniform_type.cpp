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

#include "boost/actor/config.hpp"

#define BOOST_TEST_MODULE uniform_type
#include <boost/test/included/unit_test.hpp>

#include <map>
#include <set>
#include <memory>
#include <cctype>
#include <atomic>
#include <vector>
#include <string>
#include <cstdint>
#include <cstring>
#include <sstream>
#include <iostream>
#include <iterator>
#include <algorithm>
#include <stdexcept>

#include "boost/actor/all.hpp"
#include "boost/actor/io/all.hpp"

#include "boost/actor/detail/type_nr.hpp"
#include "boost/actor/detail/uniform_type_info_map.hpp"

namespace {

struct foo {
  int value;
  explicit foo(int val = 0) : value(val) {
    // nop
  }
};

inline bool operator==(const foo& lhs, const foo& rhs) {
  return lhs.value == rhs.value;
}

enum class test_enum {
  test_value1,
  test_value2
};

struct test_struct {
  test_enum test_value;
};

} // namespace <anonymous>

using boost::none;
using boost::join;
using boost::variant;
using boost::optional;
using boost::is_any_of;
using boost::token_compress_on;
using namespace boost::actor;

template <class T>
std::string tostr(T value, std::streamsize fieldwidth = 2) {
  std::ostringstream oss;
  oss.width(fieldwidth);
  oss << value;
  return oss.str();
}

bool check_types(const std::map<std::string, uint16_t>& expected) {
  // holds the type names we see at runtime
  std::map<std::string, uint16_t> found;
  // fetch all available type names
  auto types = uniform_type_info::instances();
  for (auto tinfo : types) {
    found.emplace(tinfo->name(), tinfo->type_nr());
  }
  // compare the two maps
  if (expected == found) {
    return true;
  }
  BOOST_CHECK(false);
  using const_iterator = std::map<std::string, uint16_t>::const_iterator;
  using std::setw;
  using std::left;
  std::ostringstream oss;
  oss << left << setw(20) << ("found (" + tostr(found.size(), 1) + ")")
      << "  |  expected (" << expected.size() << ")";
  BOOST_TEST_MESSAGE(oss.str());
  oss.seekp(0);
  oss << std::setfill('-') << setw(22) << "" << "|" << setw(22) << "";
  BOOST_TEST_MESSAGE(oss.str());
  auto fi = found.cbegin();
  auto fe = found.cend();
  auto ei = expected.cbegin();
  auto ee = expected.cend();
  std::string dummy(20, ' ');
  auto out = [&](const_iterator& i, const_iterator last) -> std::string {
    if (i == last) {
      return dummy;
    }
    std::ostringstream tmp;
    tmp << left << setw(16) << i->first << "[" << tostr(i->second) << "]";
    ++i;
    return tmp.str();
  };
  while (fi != fe || ei != ee) {
    BOOST_TEST_MESSAGE(out(fi, fe) << "  |  " << out(ei, ee));
  }
  return false;
}

template <class T>
T& append(T& storage) {
  return storage;
}

template <class T, typename U, class... Us>
T& append(T& storage, U&& u, Us&&... us) {
  storage.emplace(std::forward<U>(u), uint16_t{0});
  return append(storage, std::forward<Us>(us)...);
}

template <class T>
constexpr uint16_t tnr() {
  return detail::type_nr<T>::value;
}

BOOST_AUTO_TEST_CASE(test_uniform_type) {
  auto announce1 = announce<foo>("foo", &foo::value);
  auto announce2 = announce<foo>("foo", &foo::value);
  auto announce3 = announce<foo>("foo", &foo::value);
  auto announce4 = announce<foo>("foo", &foo::value);
  BOOST_CHECK(announce1 == announce2);
  BOOST_CHECK(announce1 == announce3);
  BOOST_CHECK(announce1 == announce4);
  BOOST_CHECK(strcmp(announce1->name(), "foo") == 0);
  {
    auto uti = uniform_typeid<atom_value>();
    BOOST_CHECK(uti != nullptr);
    BOOST_CHECK(strcmp(uti->name(), "@atom") == 0);
  }
  auto sptr = detail::singletons::get_uniform_type_info_map();
  // this kind of message is used during the initialization process of BASP;
  // when compiling with logging enabled, this type ends up in the type info map
  sptr->by_uniform_name("@<>+@atom+@str");
  using detail::type_nr;
  // these types (and only those) are present if
  // the uniform_type_info implementation is correct
  std::map<std::string, uint16_t> expected{
    // local types
    {"foo", 0},
    // primitive types
    {"bool", tnr<bool>()},
    // signed integer names
    {"@i8", tnr<int8_t>()},
    {"@i16", tnr<int16_t>()},
    {"@i32", tnr<int32_t>()},
    {"@i64", tnr<int64_t>()},
    // unsigned integer names
    {"@u8", tnr<uint8_t>()},
    {"@u16", tnr<uint16_t>()},
    {"@u32", tnr<uint32_t>()},
    {"@u64", tnr<uint64_t>()},
    // strings
    {"@str", tnr<std::string>()},
    {"@u16str", tnr<std::u16string>()},
    {"@u32str", tnr<std::u32string>()},
    // floating points
    {"float", tnr<float>()},
    {"double", tnr<double>()},
    {"@ldouble", tnr<long double>()},
    // default announced types
    {"@<>", 0},
    {"@<>+@atom", 0},
    {"@<>+@atom+@str", 0},
    {"@unit", tnr<unit_t>()},
    {"@actor", tnr<actor>()},
    {"@actorvec", tnr<std::vector<actor>>()},
    {"@addr", tnr<actor_addr>()},
    {"@addrvec", tnr<std::vector<actor_addr>>()},
    {"@atom", tnr<atom_value>()},
    {"@channel", tnr<channel>()},
    {"@charbuf", tnr<std::vector<char>>()},
    {"@down", tnr<down_msg>()},
    {"@duration", tnr<duration>()},
    {"@exit", tnr<exit_msg>()},
    {"@group", tnr<group>()},
    {"@group_down", tnr<group_down_msg>()},
    {"@message", tnr<message>()},
    {"@message_id", tnr<message_id>()},
    {"@node", tnr<node_id>()},
    {"@strmap", tnr<std::map<std::string,std::string>>()},
    {"@timeout", tnr<timeout_msg>()},
    {"@sync_exited", tnr<sync_exited_msg>()},
    {"@sync_timeout", tnr<sync_timeout_msg>()},
    {"@strvec", tnr<std::vector<std::string>>()},
    {"@strset", tnr<std::set<std::string>>()}
  };
  sptr->by_uniform_name("@<>");
  sptr->by_uniform_name("@<>+@atom");
  BOOST_TEST_MESSAGE("Added debug types");
  if (check_types(expected)) {
    BOOST_TEST_MESSAGE("`check_types` succeeded");
    // causes the middleman to create its singleton
    io::middleman::instance();
    BOOST_TEST_MESSAGE("middleman instance created");
    // ok, check whether middleman announces its types correctly
    check_types(append(expected,
                       "boost::actor::io::accept_handle",
                       "boost::actor::io::acceptor_closed_msg",
                       "boost::actor::io::connection_handle",
                       "boost::actor::io::connection_closed_msg",
                       "boost::actor::io::network::protocol",
                       "boost::actor::io::new_connection_msg",
                       "boost::actor::io::new_data_msg",
                       "boost::actor::io::network::address_listing"));
    BOOST_TEST_MESSAGE("io types checked");
  }
  // check whether enums can be announced as members
  announce<test_enum>("test_enum");
  announce<test_struct>("test_struct", &test_struct::test_value);
  check_types(append(expected, "test_enum", "test_struct"));
  shutdown();
}
