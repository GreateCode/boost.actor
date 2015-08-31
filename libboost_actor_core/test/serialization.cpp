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

#define BOOST_TEST_MODULE serialization
#include <boost/test/included/unit_test.hpp>

#include <new>
#include <set>
#include <list>
#include <stack>
#include <tuple>
#include <locale>
#include <memory>
#include <string>
#include <limits>
#include <vector>
#include <cstring>
#include <sstream>
#include <cstdint>
#include <cstring>
#include <cassert>
#include <iterator>
#include <typeinfo>
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <functional>
#include <type_traits>

#include "boost/actor/message.hpp"
#include "boost/actor/announce.hpp"
#include "boost/actor/shutdown.hpp"
#include "boost/actor/to_string.hpp"
#include "boost/actor/serializer.hpp"
#include "boost/actor/from_string.hpp"
#include "boost/actor/ref_counted.hpp"
#include "boost/actor/deserializer.hpp"
#include "boost/actor/actor_namespace.hpp"
#include "boost/actor/primitive_variant.hpp"
#include "boost/actor/binary_serializer.hpp"
#include "boost/actor/binary_deserializer.hpp"
#include "boost/actor/await_all_actors_done.hpp"
#include "boost/actor/abstract_uniform_type_info.hpp"

#include "boost/actor/detail/ieee_754.hpp"
#include "boost/actor/detail/int_list.hpp"
#include "boost/actor/detail/safe_equal.hpp"
#include "boost/actor/detail/type_traits.hpp"
#include "boost/actor/detail/get_mac_addresses.hpp"

using namespace std;
using boost::none;
using boost::join;
using boost::variant;
using boost::optional;
using boost::is_any_of;
using boost::token_compress_on;
using namespace boost::actor;

namespace {

using strmap = map<string, u16string>;

struct raw_struct {
  string str;
};

bool operator==(const raw_struct& lhs, const raw_struct& rhs) {
  return lhs.str == rhs.str;
}

struct raw_struct_type_info : abstract_uniform_type_info<raw_struct> {
  using super = abstract_uniform_type_info<raw_struct>;
  raw_struct_type_info() : super("raw_struct") {
    // nop
  }
  void serialize(const void* ptr, serializer* sink) const override {
    auto rs = reinterpret_cast<const raw_struct*>(ptr);
    sink->write_value(static_cast<uint32_t>(rs->str.size()));
    sink->write_raw(rs->str.size(), rs->str.data());
  }
  void deserialize(void* ptr, deserializer* source) const override {
    auto rs = reinterpret_cast<raw_struct*>(ptr);
    rs->str.clear();
    auto size = source->read<uint32_t>();
    rs->str.resize(size);
    source->read_raw(size, &(rs->str[0]));
  }
};

enum class test_enum {
  a,
  b,
  c
};

struct fixture {
  int32_t i32 = -345;
  test_enum te = test_enum::b;
  string str = "Lorem ipsum dolor sit amet.";
  raw_struct rs;
  message msg;

  fixture() {
    announce<test_enum>("test_enum");
    announce(typeid(raw_struct), uniform_type_info_ptr{new raw_struct_type_info});
    rs.str.assign(string(str.rbegin(), str.rend()));
    msg = make_message(i32, te, str, rs);
  }

  ~fixture() {
    await_all_actors_done();
    shutdown();
  }
};

template <class T>
void apply(binary_serializer& bs, const T& x) {
  bs << x;
}

template <class T>
void apply(binary_deserializer& bd, T* x) {
  uniform_typeid<T>()->deserialize(x, &bd);
}

template <class T>
struct binary_util_fun {
  binary_util_fun(T& ref) : ref_(ref) {
    // nop
  }
  T& ref_;
  void operator()() const {
    // end of recursion
  }
  template <class U, class... Us>
  void operator()(U&& x, Us&&... xs) const {
    apply(ref_, x);
    (*this)(std::forward<Us>(xs)...);
  }
};

class binary_util {
public:
  template <class T, class... Ts>
  static vector<char> serialize(const T& x, const Ts&... xs) {
    vector<char> buf;
    binary_serializer bs{std::back_inserter(buf)};
    binary_util_fun<binary_serializer> f{bs};
    f(x, xs...);
    return buf;
  }

  template <class T, class... Ts>
  static void deserialize(const vector<char>& buf, T* x, Ts*... xs) {
    binary_deserializer bd{buf.data(), buf.size()};
    binary_util_fun<binary_deserializer> f{bd};
    f(x, xs...);
  }
};

struct is_message {
  explicit is_message(message& msgref) : msg(msgref) {
    // nop
  }

  message& msg;

  template <class T, class... Ts>
  bool equal(T&& v, Ts&&... vs) {
    bool ok = false;
    // work around for gcc 4.8.4 bug
    auto tup = tie(v, vs...);
    message_handler impl {
      [&](T const& u, Ts const&... us) {
        ok = tup == tie(u, us...);
      }
    };
    impl(msg);
    return ok;
  }
};

} // namespace <anonymous>

BOOST_FIXTURE_TEST_SUITE(serialization_tests, fixture)

BOOST_AUTO_TEST_CASE(test_serialization) {
  using token = std::integral_constant<int, detail::impl_id<strmap>()>;
  BOOST_CHECK_EQUAL(detail::is_iterable<strmap>::value, true);
  BOOST_CHECK_EQUAL(detail::is_stl_compliant_list<vector<int>>::value, true);
  BOOST_CHECK_EQUAL(detail::is_stl_compliant_list<strmap>::value, false);
  BOOST_CHECK_EQUAL(detail::is_stl_compliant_map<strmap>::value, true);
  BOOST_CHECK_EQUAL(detail::impl_id<strmap>(), 2);
  BOOST_CHECK_EQUAL(token::value, 2);

  auto nid = detail::singletons::get_node_id();
  auto nid_str = to_string(nid);
  BOOST_TEST_MESSAGE("nid_str = " << nid_str);
  auto nid2 = from_string<node_id>(nid_str);
  BOOST_CHECK(nid2);
  if (nid2) {
    BOOST_CHECK_EQUAL(to_string(nid), to_string(*nid2));
  }
}

BOOST_AUTO_TEST_CASE(test_ieee_754) {
  // check conversion of float
  float f1 = 3.1415925f;         // float value
  auto p1 = boost::actor::detail::pack754(f1); // packet value
  BOOST_CHECK_EQUAL(p1, 0x40490FDA);
  auto u1 = boost::actor::detail::unpack754(p1); // unpacked value
  BOOST_CHECK_EQUAL(f1, u1);
  // check conversion of double
  double f2 = 3.14159265358979311600;  // double value
  auto p2 = boost::actor::detail::pack754(f2); // packet value
  BOOST_CHECK_EQUAL(p2, 0x400921FB54442D18);
  auto u2 = boost::actor::detail::unpack754(p2); // unpacked value
  BOOST_CHECK_EQUAL(f2, u2);
}

BOOST_AUTO_TEST_CASE(test_int32_t) {
  auto buf = binary_util::serialize(i32);
  int32_t x;
  binary_util::deserialize(buf, &x);
  BOOST_CHECK_EQUAL(i32, x);
}

BOOST_AUTO_TEST_CASE(test_enum_serialization) {
  auto buf = binary_util::serialize(te);
  test_enum x;
  binary_util::deserialize(buf, &x);
  BOOST_CHECK(te == x);
}

BOOST_AUTO_TEST_CASE(test_string) {
  auto buf = binary_util::serialize(str);
  string x;
  binary_util::deserialize(buf, &x);
  BOOST_CHECK_EQUAL(str, x);
}

BOOST_AUTO_TEST_CASE(test_raw_struct) {
  auto buf = binary_util::serialize(rs);
  raw_struct x;
  binary_util::deserialize(buf, &x);
  BOOST_CHECK(rs == x);
}

BOOST_AUTO_TEST_CASE(test_single_message) {
  auto buf = binary_util::serialize(msg);
  message x;
  binary_util::deserialize(buf, &x);
  BOOST_CHECK(msg == x);
  BOOST_CHECK(is_message(x).equal(i32, te, str, rs));
}

BOOST_AUTO_TEST_CASE(test_multiple_messages) {
  auto m = make_message(rs, te);
  auto buf = binary_util::serialize(te, m, msg);
  test_enum t;
  message m1;
  message m2;
  binary_util::deserialize(buf, &t, &m1, &m2);
  BOOST_CHECK(tie(t, m1, m2) == tie(te, m, msg));
  BOOST_CHECK(is_message(m1).equal(rs, te));
  BOOST_CHECK(is_message(m2).equal(i32, te, str, rs));
}

BOOST_AUTO_TEST_CASE(strings) {
  auto m1 = make_message("hello \"actor world\"!", atom("foo"));
  auto s1 = to_string(m1);
  BOOST_CHECK_EQUAL(s1, "@<>+@str+@atom ( \"hello \\\"actor world\\\"!\", 'foo' )");
  BOOST_CHECK(from_string<message>(s1) == m1);
  auto m2 = make_message(true);
  auto s2 = to_string(m2);
  BOOST_CHECK_EQUAL(s2, "@<>+bool ( true )");
}

BOOST_AUTO_TEST_SUITE_END()
