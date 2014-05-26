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
 * Copyright (C) 2011 - 2014                                                  *
 * Dominik Charousset <dominik.charousset (at) haw-hamburg.de>                *
 *                                                                            *
 * Distributed under the Boost Software License, Version 1.0. See             *
 * accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt  *
\******************************************************************************/


#include <cstdio>
#include <cstring>
#include <sstream>

#include <unistd.h>
#include <sys/types.h>

#include "boost/algorithm/string.hpp" // boost::join

#include "boost/actor/config.hpp"
#include "boost/actor/node_id.hpp"
#include "boost/actor/serializer.hpp"
#include "boost/actor/primitive_variant.hpp"

#include "boost/actor_io/middleman.hpp"

#include "boost/actor/detail/singletons.hpp"
#include "boost/actor/detail/ripemd_160.hpp"
#include "boost/actor/detail/safe_equal.hpp"
#include "boost/actor/detail/get_root_uuid.hpp"
#include "boost/actor/detail/get_mac_addresses.hpp"

namespace {

std::uint8_t hex_char_value(char c) {
    if (isdigit(c)) {
        return static_cast<std::uint8_t>(c - '0');
    }
    else if (isalpha(c)) {
        if (c >= 'a' && c <= 'f') {
            return static_cast<std::uint8_t>((c - 'a') + 10);
        }
        else if (c >= 'A' && c <= 'F') {
            return static_cast<std::uint8_t>((c - 'A') + 10);
        }
    }
    throw std::invalid_argument(std::string("illegal character: ") + c);
}

} // namespace <anonymous>

namespace boost {
namespace actor {

void host_id_from_string(const std::string& hash,
                         node_id::host_id_type& node_id) {
    if (hash.size() != (node_id.size() * 2)) {
        throw std::invalid_argument("string argument is not a node id hash");
    }
    auto j = hash.c_str();
    for (size_t i = 0; i < node_id.size(); ++i) {
        // read two characters, each representing 4 bytes
        auto& val = node_id[i];
        val  = static_cast<uint8_t>(hex_char_value(*j++) << 4);
        val |= hex_char_value(*j++);
    }
}

bool equal(const std::string& hash,
           const node_id::host_id_type& node_id) {
    if (hash.size() != (node_id.size() * 2)) {
        return false;
    }
    auto j = hash.c_str();
    try {
        for (size_t i = 0; i < node_id.size(); ++i) {
            // read two characters, each representing 4 bytes
            std::uint8_t val;
            val  = static_cast<uint8_t>(hex_char_value(*j++) << 4);
            val |= hex_char_value(*j++);
            if (val != node_id[i]) {
                return false;
            }
        }
    }
    catch (std::invalid_argument&) {
        return false;
    }
    return true;
}

node_id::~node_id() { }

node_id::node_id(const node_id& other)
: super(), m_process_id(other.process_id()), m_host_id(other.host_id()) { }

node_id::node_id(std::uint32_t a, const std::string& b)
: m_process_id(a) {
    host_id_from_string(b, m_host_id);
}

node_id::node_id(std::uint32_t a, const host_id_type& b)
: m_process_id(a), m_host_id(b) { }

std::string to_string(const node_id::host_id_type& node_id) {
    std::ostringstream oss;
    oss << std::hex;
    oss.fill('0');
    for (size_t i = 0; i < node_id::host_id_size; ++i) {
        oss.width(2);
        oss << static_cast<std::uint32_t>(node_id[i]);
    }
    return oss.str();
}

int node_id::compare(const node_id& other) const {
    int tmp = strncmp(reinterpret_cast<const char*>(host_id().data()),
                      reinterpret_cast<const char*>(other.host_id().data()),
                      host_id_size);
    if (tmp == 0) {
        if (m_process_id < other.process_id()) return -1;
        else if (m_process_id == other.process_id()) return 0;
        return 1;
    }
    return tmp;
}

void node_id::serialize_invalid(serializer* sink) {
    sink->write_value(static_cast<uint32_t>(0));
    node_id::host_id_type zero;
    std::fill(zero.begin(), zero.end(), 0);
    sink->write_raw(node_id::host_id_size, zero.data());
}

std::string to_string(const node_id& what) {
    std::ostringstream oss;
    oss << what.process_id() << "@" << to_string(what.host_id());
    return oss.str();
}

std::string to_string(const node_id_ptr& what) {
    std::ostringstream oss;
    oss << "@process_info(";
    if (!what) oss << "null";
    else oss << to_string(*what);
    oss << ")";
    return oss.str();
}

// initializes singleton
node_id* node_id::create_singleton() {
    auto macs = detail::get_mac_addresses();
    auto hd_serial_and_mac_addr = join(macs, "")
                                + detail::get_root_uuid();
    node_id::host_id_type nid;
    detail::ripemd_160(nid, hd_serial_and_mac_addr);
    auto ptr = new node_id(static_cast<uint32_t>(getpid()), nid);
    ptr->ref(); // implicit ref count held by detail::singletons
    return ptr;
}

} // namespace actor
} // namespace boost
