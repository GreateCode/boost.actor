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

#define BOOST_TEST_MODULE ripemd_160
#include <boost/test/included/unit_test.hpp>

#include <iomanip>
#include <iostream>

#include "boost/actor/detail/ripemd_160.hpp"

using boost::actor::detail::ripemd_160;

namespace {

std::string str_hash(const std::string& what) {
  std::array<uint8_t, 20> hash;
  ripemd_160(hash, what);
  std::ostringstream oss;
  oss << std::setfill('0') << std::hex;
  for (auto i : hash) {
    oss << std::setw(2) << static_cast<int>(i);
  }
  return oss.str();
}

} // namespace <anonymous>

// verify ripemd implementation with example hash results from
// http://homes.esat.kuleuven.be/~bosselae/ripemd160.html
BOOST_AUTO_TEST_CASE(hash_results) {
  BOOST_CHECK_EQUAL("9c1185a5c5e9fc54612808977ee8f548b2258d31", str_hash(""));
  BOOST_CHECK_EQUAL("0bdc9d2d256b3ee9daae347be6f4dc835a467ffe", str_hash("a"));
  BOOST_CHECK_EQUAL("8eb208f7e05d987a9b044a8e98c6b087f15a0bfc", str_hash("abc"));
  BOOST_CHECK_EQUAL("5d0689ef49d2fae572b881b123a85ffa21595f36",
                  str_hash("message digest"));
  BOOST_CHECK_EQUAL("f71c27109c692c1b56bbdceb5b9d2865b3708dbc",
                  str_hash("abcdefghijklmnopqrstuvwxyz"));
  BOOST_CHECK_EQUAL("12a053384a9c0c88e405a06c27dcf49ada62eb2b",
                  str_hash("abcdbcdecdefdefgefghfghighij"
                           "hijkijkljklmklmnlmnomnopnopq"));
  BOOST_CHECK_EQUAL("b0e20b6e3116640286ed3a87a5713079b21f5189",
                  str_hash("ABCDEFGHIJKLMNOPQRSTUVWXYZabcde"
                           "fghijklmnopqrstuvwxyz0123456789"));
  BOOST_CHECK_EQUAL("9b752e45573d4b39f4dbd3323cab82bf63326bfb",
                  str_hash("1234567890123456789012345678901234567890"
                           "1234567890123456789012345678901234567890"));
}
