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

#ifndef BOOST_ACTOR_FWD_HPP
#define BOOST_ACTOR_FWD_HPP

#include <memory>
#include <cstdint>

namespace boost {
namespace actor {

// skip intrusive_ptr<> and optional<> in Boost.Actor

// classes
class actor;
class group;
class message;
class channel;
class node_id;
class duration;
class behavior;
class resumable;
class actor_addr;
class actor_pool;
class message_id;
class serializer;
class local_actor;
class actor_proxy;
class deserializer;
class scoped_actor;
class execution_unit;
class abstract_actor;
class abstract_group;
class blocking_actor;
class mailbox_element;
class message_handler;
class uniform_type_info;
class event_based_actor;
class binary_serializer;
class binary_deserializer;
class forwarding_actor_proxy;

// structs
struct unit_t;
struct anything;
struct exit_msg;
struct down_msg;
struct timeout_msg;
struct group_down_msg;
struct invalid_actor_t;
struct sync_exited_msg;
struct sync_timeout_msg;
struct invalid_actor_addr_t;
struct illegal_message_element;

// enums
enum class atom_value : uint64_t;

// aliases
using actor_id = uint32_t;

// functions
template <class T, typename U>
T actor_cast(const U&);

namespace io {

class broker;
class middleman;

namespace basp {

struct header;

} // namespace basp

} // namespace io

namespace scheduler {
  class abstract_worker;
  class abstract_coordinator;
} // namespace scheduler

namespace detail {
  class logging;
  class disposer;
  class singletons;
  class message_data;
  class group_manager;
  class actor_registry;
  class uniform_type_info_map;
} // namespace detail

using mailbox_element_ptr = std::unique_ptr<mailbox_element, detail::disposer>;

} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_FWD_HPP
