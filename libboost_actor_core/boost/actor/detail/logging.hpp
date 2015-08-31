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

#ifndef BOOST_ACTOR_DETAIL_LOGGING_HPP
#define BOOST_ACTOR_DETAIL_LOGGING_HPP

#include <thread>
#include <cstring>
#include <sstream>
#include <iostream>
#include <type_traits>
#include <unordered_map>

#include "boost/actor/config.hpp"
#include "boost/actor/to_string.hpp"
#include "boost/actor/abstract_actor.hpp"

#include "boost/actor/detail/singletons.hpp"
#include "boost/actor/detail/scope_guard.hpp"
#include "boost/actor/detail/shared_spinlock.hpp"

/*
 * To enable logging, you have to define BOOST_ACTOR_DEBUG. This enables
 * BOOST_ACTOR_LOG_ERROR messages. To enable more debugging output, you can
 * define BOOST_ACTOR_LOG_LEVEL to:
 * 1: + warning
 * 2: + info
 * 3: + debug
 * 4: + trace (prints for each logged method entry and exit message)
 *
 * Note: this logger emits log4j style XML output; logs are best viewed
 *       using a log4j viewer, e.g., http://code.google.com/p/otroslogviewer/
 *
 */
namespace boost {
namespace actor {
namespace detail {

class singletons;

class logging {
public:
  friend class detail::singletons;

  // returns the actor ID for the current thread or 0 if none is assigned
  actor_id get_aid();

  // associates given actor id with this thread,
  // returns the previously set actor id
  actor_id set_aid(actor_id aid);

  virtual void log(const char* level, const char* class_name,
                   const char* function_name, const char* file_name,
                   int line_num, const std::string& msg) = 0;

  class trace_helper {
  public:
    trace_helper(std::string class_name, const char* fun_name,
                 const char* file_name, int line_num, const std::string& msg);

    ~trace_helper();

  private:
    std::string class_;
    const char* fun_name_;
    const char* file_name_;
    int line_num_;
  };

protected:
  virtual ~logging();

  static logging* create_singleton();

  virtual void initialize() = 0;

  virtual void stop() = 0;

  inline void dispose() {
    delete this;
  }

private:
  detail::shared_spinlock aids_lock_;
  std::unordered_map<std::thread::id, actor_id> aids_;
};

struct oss_wr {
  inline oss_wr() {
    // nop
  }

  inline oss_wr(oss_wr&& other) : str_(std::move(other.str_)) {
    // nop
  }

  std::string str_;

  inline std::string str() {
    return std::move(str_);
  }
};

inline oss_wr operator<<(oss_wr&& lhs, std::string str) {
  lhs.str_ += std::move(str);
  return std::move(lhs);
}

inline oss_wr operator<<(oss_wr&& lhs, const char* str) {
  lhs.str_ += str;
  return std::move(lhs);
}

template <class T>
oss_wr operator<<(oss_wr&& lhs, T rhs) {
  std::ostringstream oss;
  oss << rhs;
  lhs.str_ += oss.str();
  return std::move(lhs);
}

} // namespace detail
} // namespace actor
} // namespace boost

#define BOOST_ACTOR_VOID_STMT static_cast<void>(0)

#define BOOST_ACTOR_CAT(a, b) a##b

#define BOOST_ACTOR_ERROR 0
#define BOOST_ACTOR_WARNING 1
#define BOOST_ACTOR_INFO 2
#define BOOST_ACTOR_DEBUG 3
#define BOOST_ACTOR_TRACE 4

#define BOOST_ACTOR_LVL_NAME0() "ERROR"
#define BOOST_ACTOR_LVL_NAME1() "WARN "
#define BOOST_ACTOR_LVL_NAME2() "INFO "
#define BOOST_ACTOR_LVL_NAME3() "DEBUG"
#define BOOST_ACTOR_LVL_NAME4() "TRACE"

#define BOOST_ACTOR_CONCAT_(LHS, RHS) LHS ## RHS
#define BOOST_ACTOR_CONCAT(LHS, RHS) BOOST_ACTOR_CONCAT_(LHS, RHS)
#define BOOST_ACTOR_UNIFYN(NAME) BOOST_ACTOR_CONCAT(NAME, __LINE__)

#define BOOST_ACTOR_PRINT_ERROR_IMPL(lvlname, classname, funname, message)             \
  {                                                                            \
    std::cerr << "[" << lvlname << "] " << classname << "::" << funname        \
              << ": " << message << "\n";                                      \
  }                                                                            \
  BOOST_ACTOR_VOID_STMT

#ifndef BOOST_ACTOR_LOG_LEVEL
inline boost::actor::actor_id caf_set_aid_dummy() { return 0; }
#define BOOST_ACTOR_LOG_IMPL(lvlname, classname, funname, message)                     \
  BOOST_ACTOR_PRINT_ERROR_IMPL(lvlname, classname, funname, message)
#define BOOST_ACTOR_PUSH_AID(unused) static_cast<void>(0)
#define BOOST_ACTOR_PUSH_AID_FROM_PTR(unused) static_cast<void>(0)
#define BOOST_ACTOR_SET_AID(unused) caf_set_aid_dummy()
#else
#define BOOST_ACTOR_LOG_IMPL(lvlname, classname, funname, message)                     \
  if (strcmp(lvlname, "ERROR") == 0) {                                         \
    BOOST_ACTOR_PRINT_ERROR_IMPL(lvlname, classname, funname, message);                \
  }                                                                            \
  boost::actor::detail::singletons::get_logger()->log(lvlname, classname, funname,      \
                                             __FILE__, __LINE__,               \
                                             (boost::actor::detail::oss_wr{}            \
                                              << std::boolalpha                \
                                              << message).str())
#define BOOST_ACTOR_PUSH_AID(aid_arg)                                                  \
  auto BOOST_ACTOR_UNIFYN(caf_aid_tmp)                                                 \
    = boost::actor::detail::singletons::get_logger()->set_aid(aid_arg);                 \
  auto BOOST_ACTOR_UNIFYN(aid_aid_tmp_guard) = boost::actor::detail::make_scope_guard([=] {     \
    boost::actor::detail::singletons::get_logger()->set_aid(BOOST_ACTOR_UNIFYN(caf_aid_tmp));   \
  })
#define BOOST_ACTOR_PUSH_AID_FROM_PTR(some_ptr)                                        \
  auto BOOST_ACTOR_UNIFYN(caf_aid_ptr) = some_ptr;                                     \
  BOOST_ACTOR_PUSH_AID(BOOST_ACTOR_UNIFYN(caf_aid_ptr) ? BOOST_ACTOR_UNIFYN(caf_aid_ptr)->id() : 0)
#define BOOST_ACTOR_SET_AID(aid_arg)                                                   \
  boost::actor::detail::singletons::get_logger()->set_aid(aid_arg)
#endif

#define BOOST_ACTOR_PRINT0(lvlname, classname, funname, msg)                           \
  BOOST_ACTOR_LOG_IMPL(lvlname, classname, funname, msg)

#define BOOST_ACTOR_PRINT_IF0(stmt, lvlname, classname, funname, msg)                  \
  if (stmt) {                                                                  \
    BOOST_ACTOR_LOG_IMPL(lvlname, classname, funname, msg);                            \
  }                                                                            \
  BOOST_ACTOR_VOID_STMT

#define BOOST_ACTOR_PRINT1(lvlname, classname, funname, msg)                           \
  BOOST_ACTOR_PRINT0(lvlname, classname, funname, msg)

#define BOOST_ACTOR_PRINT_IF1(stmt, lvlname, classname, funname, msg)                  \
  BOOST_ACTOR_PRINT_IF0(stmt, lvlname, classname, funname, msg)

#if ! defined(BOOST_ACTOR_LOG_LEVEL) || BOOST_ACTOR_LOG_LEVEL < BOOST_ACTOR_TRACE
#define BOOST_ACTOR_PRINT4(arg0, arg1, arg2, arg3)
#else
#define BOOST_ACTOR_PRINT4(lvlname, classname, funname, msg)                           \
  boost::actor::detail::logging::trace_helper BOOST_ACTOR_UNIFYN(caf_log_trace_)  {             \
    classname, funname, __FILE__, __LINE__,                                    \
      (boost::actor::detail::oss_wr{} << msg).str()                                     \
  }
#endif

#if ! defined(BOOST_ACTOR_LOG_LEVEL) || BOOST_ACTOR_LOG_LEVEL < BOOST_ACTOR_DEBUG
#define BOOST_ACTOR_PRINT3(arg0, arg1, arg2, arg3)
#define BOOST_ACTOR_PRINT_IF3(arg0, arg1, arg2, arg3, arg4)
#else
#define BOOST_ACTOR_PRINT3(lvlname, classname, funname, msg)                           \
  BOOST_ACTOR_PRINT0(lvlname, classname, funname, msg)
#define BOOST_ACTOR_PRINT_IF3(stmt, lvlname, classname, funname, msg)                  \
  BOOST_ACTOR_PRINT_IF0(stmt, lvlname, classname, funname, msg)
#endif

#if ! defined(BOOST_ACTOR_LOG_LEVEL) || BOOST_ACTOR_LOG_LEVEL < BOOST_ACTOR_INFO
#define BOOST_ACTOR_PRINT2(arg0, arg1, arg2, arg3)
#define BOOST_ACTOR_PRINT_IF2(arg0, arg1, arg2, arg3, arg4)
#else
#define BOOST_ACTOR_PRINT2(lvlname, classname, funname, msg)                           \
  BOOST_ACTOR_PRINT0(lvlname, classname, funname, msg)
#define BOOST_ACTOR_PRINT_IF2(stmt, lvlname, classname, funname, msg)                  \
  BOOST_ACTOR_PRINT_IF0(stmt, lvlname, classname, funname, msg)
#endif

#define BOOST_ACTOR_EVAL(what) what

/// @def BOOST_ACTOR_LOGC
/// Logs a message with custom class and function names.
#define BOOST_ACTOR_LOGC(level, classname, funname, msg)                               \
  BOOST_ACTOR_CAT(BOOST_ACTOR_PRINT, level)(BOOST_ACTOR_CAT(BOOST_ACTOR_LVL_NAME, level)(), classname,         \
                            funname, msg)

/// @def BOOST_ACTOR_LOGF
/// Logs a message inside a free function.
#define BOOST_ACTOR_LOGF(level, msg) BOOST_ACTOR_LOGC(level, "NONE", __func__, msg)

/// @def BOOST_ACTOR_LOGMF
/// Logs a message inside a member function.
#define BOOST_ACTOR_LOGMF(level, msg)                                                  \
  BOOST_ACTOR_LOGC(level, typeid(*this).name(), __func__, msg)

/// @def BOOST_ACTOR_LOGC
/// Logs a message with custom class and function names.
#define BOOST_ACTOR_LOGC_IF(stmt, level, classname, funname, msg)                      \
  BOOST_ACTOR_CAT(BOOST_ACTOR_PRINT_IF, level)(stmt, BOOST_ACTOR_CAT(BOOST_ACTOR_LVL_NAME, level)(),           \
                               classname, funname, msg)

/// @def BOOST_ACTOR_LOGF
/// Logs a message inside a free function.
#define BOOST_ACTOR_LOGF_IF(stmt, level, msg)                                          \
  BOOST_ACTOR_LOGC_IF(stmt, level, "NONE", __func__, msg)

/// @def BOOST_ACTOR_LOGMF
/// Logs a message inside a member function.
#define BOOST_ACTOR_LOGMF_IF(stmt, level, msg)                                         \
  BOOST_ACTOR_LOGC_IF(stmt, level, typeid(*this).name(), __func__, msg)

// convenience macros to safe some typing when printing arguments
#define BOOST_ACTOR_ARG(arg) #arg << " = " << arg
#define BOOST_ACTOR_TARG(arg, trans) #arg << " = " << trans(arg)
#define BOOST_ACTOR_MARG(arg, memfun) #arg << " = " << arg.memfun()
#define BOOST_ACTOR_TSARG(arg) #arg << " = " << to_string(arg)

/******************************************************************************
 *                             convenience macros                             *
 ******************************************************************************/

#define BOOST_ACTOR_LOG_ERROR(msg) BOOST_ACTOR_LOGMF(BOOST_ACTOR_ERROR, msg)
#define BOOST_ACTOR_LOG_WARNING(msg) BOOST_ACTOR_LOGMF(BOOST_ACTOR_WARNING, msg)
#define BOOST_ACTOR_LOG_DEBUG(msg) BOOST_ACTOR_LOGMF(BOOST_ACTOR_DEBUG, msg)
#define BOOST_ACTOR_LOG_INFO(msg) BOOST_ACTOR_LOGMF(BOOST_ACTOR_INFO, msg)
#define BOOST_ACTOR_LOG_TRACE(msg) BOOST_ACTOR_LOGMF(BOOST_ACTOR_TRACE, msg)

#define BOOST_ACTOR_LOG_ERROR_IF(stmt, msg) BOOST_ACTOR_LOGMF_IF(stmt, BOOST_ACTOR_ERROR, msg)
#define BOOST_ACTOR_LOG_WARNING_IF(stmt, msg) BOOST_ACTOR_LOGMF_IF(stmt, BOOST_ACTOR_WARNING, msg)
#define BOOST_ACTOR_LOG_DEBUG_IF(stmt, msg) BOOST_ACTOR_LOGMF_IF(stmt, BOOST_ACTOR_DEBUG, msg)
#define BOOST_ACTOR_LOG_INFO_IF(stmt, msg) BOOST_ACTOR_LOGMF_IF(stmt, BOOST_ACTOR_INFO, msg)
#define BOOST_ACTOR_LOG_TRACE_IF(stmt, msg) BOOST_ACTOR_LOGMF_IF(stmt, BOOST_ACTOR_TRACE, msg)

#define BOOST_ACTOR_LOGF_ERROR(msg) BOOST_ACTOR_LOGF(BOOST_ACTOR_ERROR, msg)
#define BOOST_ACTOR_LOGF_WARNING(msg) BOOST_ACTOR_LOGF(BOOST_ACTOR_WARNING, msg)
#define BOOST_ACTOR_LOGF_DEBUG(msg) BOOST_ACTOR_LOGF(BOOST_ACTOR_DEBUG, msg)
#define BOOST_ACTOR_LOGF_INFO(msg) BOOST_ACTOR_LOGF(BOOST_ACTOR_INFO, msg)
#define BOOST_ACTOR_LOGF_TRACE(msg) BOOST_ACTOR_LOGF(BOOST_ACTOR_TRACE, msg)

#define BOOST_ACTOR_LOGF_ERROR_IF(stmt, msg) BOOST_ACTOR_LOGF_IF(stmt, BOOST_ACTOR_ERROR, msg)
#define BOOST_ACTOR_LOGF_WARNING_IF(stmt, msg) BOOST_ACTOR_LOGF_IF(stmt, BOOST_ACTOR_WARNING, msg)
#define BOOST_ACTOR_LOGF_DEBUG_IF(stmt, msg) BOOST_ACTOR_LOGF_IF(stmt, BOOST_ACTOR_DEBUG, msg)
#define BOOST_ACTOR_LOGF_INFO_IF(stmt, msg) BOOST_ACTOR_LOGF_IF(stmt, BOOST_ACTOR_INFO, msg)
#define BOOST_ACTOR_LOGF_TRACE_IF(stmt, msg) BOOST_ACTOR_LOGF_IF(stmt, BOOST_ACTOR_TRACE, msg)

#endif // BOOST_ACTOR_DETAIL_LOGGING_HPP
