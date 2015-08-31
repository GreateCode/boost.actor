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

#define BOOST_TEST_MODULE extract
#include <boost/test/included/unit_test.hpp>

#include <string>
#include <vector>

#include "boost/actor/all.hpp"

namespace boost {
namespace actor {

std::ostream& operator<<(std::ostream& out, const message& msg) {
  return out << to_string(msg);
}

} // namespace actor
} // namespace boost

using boost::none;
using boost::join;
using boost::variant;
using boost::optional;
using boost::is_any_of;
using boost::token_compress_on;
using namespace boost::actor;

using std::string;

// exclude this test; advanced match cases are currently not supported on MSVC
#ifndef BOOST_ACTOR_WINDOWS
BOOST_AUTO_TEST_CASE(simple_ints) {
  auto msg = make_message(1, 2, 3);
  auto one = on(1) >> [] { };
  auto two = on(2) >> [] { };
  auto three = on(3) >> [] { };
  auto skip_two = [](int i) -> optional<skip_message_t> {
    if (i == 2) {
      return skip_message();
    }
    return none;
  };
  BOOST_CHECK_EQUAL(msg.extract(one), make_message(2, 3));
  BOOST_CHECK_EQUAL(msg.extract(two), make_message(1, 3));
  BOOST_CHECK_EQUAL(msg.extract(three), make_message(1, 2));
  BOOST_CHECK_EQUAL(msg.extract(skip_two), make_message(2));
}
#endif // BOOST_ACTOR_WINDOWS

BOOST_AUTO_TEST_CASE(type_sequences) {
  auto _64 = uint64_t{64};
  auto msg = make_message(1.0, 2.f, "str", 42, _64);
  auto df = [](double, float) { };
  auto fs = [](float, const string&) { };
  auto iu = [](int, uint64_t) { };
  BOOST_CHECK_EQUAL(msg.extract(df), make_message("str", 42,  _64));
  BOOST_CHECK_EQUAL(msg.extract(fs), make_message(1.0, 42,  _64));
  BOOST_CHECK_EQUAL(msg.extract(iu), make_message(1.0, 2.f, "str"));
}

BOOST_AUTO_TEST_CASE(cli_args) {
  std::vector<string> args{"-n", "-v", "5", "--out-file=/dev/null"};
  int verbosity = 0;
  string output_file;
  string input_file;
  auto res = message_builder(args.begin(), args.end()).extract_opts({
    {"no-colors,n", "disable colors"},
    {"out-file,o", "redirect output", output_file},
    {"in-file,i", "read from file", input_file},
    {"verbosity,v", "1-5", verbosity}
  });
  BOOST_CHECK_EQUAL(res.remainder.size(), 0);
  BOOST_CHECK_EQUAL(to_string(res.remainder), to_string(message{}));
  BOOST_CHECK_EQUAL(res.opts.count("no-colors"), 1);
  BOOST_CHECK_EQUAL(res.opts.count("verbosity"), 1);
  BOOST_CHECK_EQUAL(res.opts.count("out-file"), 1);
  BOOST_CHECK_EQUAL(res.opts.count("in-file"), 0);
  BOOST_CHECK_EQUAL(output_file, "/dev/null");
  BOOST_CHECK_EQUAL(input_file, "");
}
