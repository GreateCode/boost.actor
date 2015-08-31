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

#include "boost/actor/detail/run_program.hpp"

#include <sstream>
#include <iostream>

#include "boost/actor/config.hpp"

#ifdef BOOST_ACTOR_MSVC
# include <windows.h>
#endif

namespace boost {
namespace actor {
namespace detail {

#ifndef BOOST_ACTOR_WINDOWS
std::thread run_program_impl(actor rc, const char* cpath,
                             std::vector<std::string> args) {
  using namespace std;
  string path = cpath;
  replace_all(path, "'", "\\'");
  ostringstream oss;
  oss << "'" << path << "'";
  for (auto& arg : args) {
    oss << " " << arg;
  }
  oss << " 2>&1";
  string cmdstr = oss.str();
  return std::thread{ [cmdstr, rc] {
    // on FreeBSD, popen() hangs indefinitely in some cases
#   ifdef BOOST_ACTOR_BSD
    system(cmdstr.c_str());
    anon_send(rc, "");
#   else
    string output;
    auto fp = popen(cmdstr.c_str(), "r");
    if (! fp) {
      cerr << "FATAL: command line failed: " << cmdstr
           << endl;
      abort();
    }
    char buf[512];
    while (fgets(buf, sizeof(buf), fp)) {
      output += buf;
    }
    pclose(fp);
    anon_send(rc, output);
#   endif
  }};
}
#else
std::thread run_program_impl(actor rc, const char* cpath,
                             std::vector<std::string> args) {
  std::string path = cpath;
  replace_all(path, "'", "\\'");
  std::ostringstream oss;
  oss << path;
  for (auto& arg : args) {
    oss << " " << arg;
  }
  return std::thread([rc](std::string cmdstr) {
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    CreateProcess(nullptr, // no module name
      &cmdstr[0], // command line
      nullptr, // process handle not inheritable
      nullptr, // thread handle not inheritable
      false, // no handle inheritance
      0, // no creation flags
      nullptr, // use parent's environment
      nullptr, // use parent's directory
      &si,
      &pi);
    // be a good parent and wait for our little child
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    anon_send(rc, "--- process output on windows not implemented yet ---");
  }, oss.str());
}
#endif

} // namespace detail
} // namespace actor
} // namespace boost
