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

#ifndef BOOST_ACTOR_NODE_ID_HPP
#define BOOST_ACTOR_NODE_ID_HPP

#include <array>
#include <string>
#include <cstdint>
#include <functional>

#include "boost/actor/fwd.hpp"
#include "boost/actor/config.hpp"
#include "boost/actor/ref_counted.hpp"
#include "boost/intrusive_ptr.hpp"

#include "boost/actor/detail/comparable.hpp"

namespace boost {
namespace actor {

class serializer;

struct invalid_node_id_t {
  constexpr invalid_node_id_t() {
    // nop
  }
};

/// Identifies an invalid {@link node_id}.
/// @relates node_id
constexpr invalid_node_id_t invalid_node_id = invalid_node_id_t{};

/// A node ID consists of a host ID and process ID. The host ID identifies
/// the physical machine in the network, whereas the process ID identifies
/// the running system-level process on that machine.
class node_id : detail::comparable<node_id>,
                detail::comparable<node_id, invalid_node_id_t> {
public:
  ~node_id();

  node_id() = default;

  node_id(const node_id&) = default;

  node_id(const invalid_node_id_t&);

  node_id& operator=(const node_id&) = default;

  node_id& operator=(const invalid_node_id_t&);

  /// A 160 bit hash (20 bytes).
  static constexpr size_t host_id_size = 20;

  /// The size of a `node_id` in serialized form.
  static constexpr size_t serialized_size = host_id_size + sizeof(uint32_t);

  /// Represents a 160 bit hash.
  using host_id_type = std::array<uint8_t, host_id_size>;

  /// Creates a node ID from `process_id` and `hash`.
  /// @param process_id System-wide unique process identifier.
  /// @param hash Unique node id as hexadecimal string representation.
  node_id(uint32_t process_id, const std::string& hash);

  /// Creates a node ID from `process_id` and `hash`.
  /// @param process_id System-wide unique process identifier.
  /// @param node_id Unique node id.
  node_id(uint32_t process_id, const host_id_type& node_id);

  /// Identifies the running process.
  /// @returns A system-wide unique process identifier.
  uint32_t process_id() const;

  /// Identifies the host system.
  /// @returns A hash build from the MAC address of the first network device
  ///      and the UUID of the root partition (mounted in "/" or "C:").
  const host_id_type& host_id() const;

  /// @cond PRIVATE

  void serialize(serializer& sink) const;

  void deserialize(deserializer& source);

  // A reference counted container for host ID and process ID.
  class data : public ref_counted {
  public:
    // for singleton API
    void stop();

    // for singleton API
    inline void dispose() {
      deref();
    }

    // for singleton API
    inline void initialize() {
      // nop
    }

    static data* create_singleton();

    int compare(const node_id& other) const;

    ~data();

    data();

    data(uint32_t procid, host_id_type hid);

    data(uint32_t procid, const std::string& hash);

    uint32_t pid_;

    host_id_type host_;
  };

  // "inherited" from comparable<node_id>
  int compare(const node_id& other) const;

  // "inherited" from comparable<node_id, invalid_node_id_t>
  int compare(const invalid_node_id_t&) const;

  explicit node_id(intrusive_ptr<data> dataptr);

  /// @endcond

private:
  intrusive_ptr<data> data_;
};

} // namespace actor
} // namespace boost

namespace std{

template<>
struct hash<boost::actor::node_id> {
  size_t operator()(const boost::actor::node_id& nid) const {
    if (nid == boost::actor::invalid_node_id)
      return 0;
    // xor the first few bytes from the node ID and the process ID
    auto x = static_cast<size_t>(nid.process_id());
    auto y = *(reinterpret_cast<const size_t*>(&nid.host_id()));
    return x ^ y;
  }
};

} // namespace std

#endif // BOOST_ACTOR_NODE_ID_HPP
