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

#define BOOST_TEST_MODULE limited_vector
#include <boost/test/included/unit_test.hpp>

#include <algorithm>

#include "boost/actor/detail/limited_vector.hpp"

using boost::actor::detail::limited_vector;

BOOST_AUTO_TEST_CASE(basics) {
  using std::equal;
  int arr1[] {1, 2, 3, 4};
  limited_vector<int, 4> vec1 {1, 2, 3, 4};
  limited_vector<int, 5> vec2 {4, 3, 2, 1};
  limited_vector<int, 4> vec3;
  for (int i = 1; i <= 4; ++i) vec3.push_back(i);
  limited_vector<int, 4> vec4 {1, 2};
  limited_vector<int, 2> vec5 {3, 4};
  vec4.insert(vec4.end(), vec5.begin(), vec5.end());
  auto vec6 = vec4;
  BOOST_CHECK_EQUAL(vec1.size(), 4);
  BOOST_CHECK_EQUAL(vec2.size(), 4);
  BOOST_CHECK_EQUAL(vec3.size(), 4);
  BOOST_CHECK_EQUAL(vec4.size(), 4);
  BOOST_CHECK_EQUAL(vec5.size(), 2);
  BOOST_CHECK_EQUAL(vec6.size(), 4);
  BOOST_CHECK_EQUAL(vec1.full(), true);
  BOOST_CHECK_EQUAL(vec2.full(), false);
  BOOST_CHECK_EQUAL(vec3.full(), true);
  BOOST_CHECK_EQUAL(vec4.full(), true);
  BOOST_CHECK_EQUAL(vec5.full(), true);
  BOOST_CHECK_EQUAL(vec6.full(), true);
  BOOST_CHECK(std::equal(vec1.begin(), vec1.end(), arr1));
  BOOST_CHECK(std::equal(vec2.rbegin(), vec2.rend(), arr1));
  BOOST_CHECK(std::equal(vec4.begin(), vec4.end(), arr1));
  BOOST_CHECK(std::equal(vec6.begin(), vec6.end(), arr1));
  BOOST_CHECK(std::equal(vec6.begin(), vec6.end(), vec2.rbegin()));
  limited_vector<int, 10> vec7 {5, 9};
  limited_vector<int, 10> vec8 {1, 2, 3, 4};
  limited_vector<int, 10> vec9 {6, 7, 8};
  vec7.insert(vec7.begin() + 1, vec9.begin(), vec9.end());
  vec7.insert(vec7.begin(), vec8.begin(), vec8.end());
  BOOST_CHECK_EQUAL(vec7.full(), false);
  limited_vector<int, 1> vec10 {10};
  vec7.insert(vec7.end(), vec10.begin(), vec10.end());
  BOOST_CHECK_EQUAL(vec7.full(), true);
  BOOST_CHECK((std::is_sorted(vec7.begin(), vec7.end())));
  int arr2[] {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  BOOST_CHECK((std::equal(vec7.begin(), vec7.end(), std::begin(arr2))));
  vec7.assign(std::begin(arr2), std::end(arr2));
  BOOST_CHECK((std::equal(vec7.begin(), vec7.end(), std::begin(arr2))));
  vec7.assign(5, 0);
  BOOST_CHECK_EQUAL(vec7.size(), 5);
  BOOST_CHECK((std::all_of(vec7.begin(), vec7.end(),
              [](int i) { return i == 0; })));
}
