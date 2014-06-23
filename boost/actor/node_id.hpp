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


#ifndef BOOST_ACTOR_PROCESS_INFORMATION_HPP
#define BOOST_ACTOR_PROCESS_INFORMATION_HPP

#include <array>
#include <string>
#include <cstdint>

#include "boost/intrusive_ptr.hpp"

#include "boost/actor/config.hpp"
#include "boost/actor/ref_counted.hpp"

#include "boost/actor/detail/comparable.hpp"

namespace boost {
namespace actor {

class serializer;

struct invalid_node_id_t { constexpr invalid_node_id_t() { } };

/**
 * @brief Identifies an invalid {@link node_id}.
 * @relates node_id
 */
constexpr invalid_node_id_t invalid_node_id = invalid_node_id_t{};

/**
 * @brief A node ID consists of a host ID and process ID. The host ID
 *        identifies the physical machine in the network, whereas the
 *        process ID identifies the running system-level process on that
 *        machine.
 */
class node_id : detail::comparable<node_id>,
                detail::comparable<node_id, invalid_node_id_t> {

    using super = ref_counted;

 public:

    node_id() = default;

    node_id(const invalid_node_id_t&);

    node_id& operator=(const node_id&) = default;

    node_id& operator=(const invalid_node_id_t&);

    /**
     * @brief A 160 bit hash (20 bytes).
     */
    static constexpr size_t host_id_size = 20;

    /**
     * @brief Represents a 160 bit hash.
     */
    typedef std::array<uint8_t, host_id_size> host_id_type;

    /**
     * @brief A reference counted container for host ID and process ID.
     */
    class data : public ref_counted {

     public:

        // for singleton API
        inline void destroy() {
            deref();
        }

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

        data() = default;

        ~data();

        data(uint32_t procid, host_id_type hid);

        uint32_t process_id;

        host_id_type host_id;

    };

    ~node_id();

    node_id(const node_id&);

    /**
     * @brief Creates @c this from @p process_id and @p hash.
     * @param process_id System-wide unique process identifier.
     * @param hash Unique node id as hexadecimal string representation.
     */
    node_id(uint32_t process_id, const std::string& hash);

    /**
     * @brief Creates @c this from @p process_id and @p hash.
     * @param process_id System-wide unique process identifier.
     * @param node_id Unique node id.
     */
    node_id(uint32_t process_id, const host_id_type& node_id);

    /**
     * @brief Identifies the running process.
     * @returns A system-wide unique process identifier.
     */
    uint32_t process_id() const;

    /**
     * @brief Identifies the host system.
     * @returns A hash build from the MAC address of the first network device
     *          and the UUID of the root partition (mounted in "/" or "C:").
     */
    const host_id_type& host_id() const;

    /** @cond PRIVATE */

    // "inherited" from comparable<node_id>
    int compare(const node_id& other) const;

    // "inherited" from comparable<node_id, invalid_node_id_t>
    int compare(const invalid_node_id_t&) const;

    node_id(intrusive_ptr<data> dataptr);

    /** @endcond */

 private:

    intrusive_ptr<data> m_data;

};

/**
 * @relates node_id
 */
std::string to_string(const node_id& what);

} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_PROCESS_INFORMATION_HPP
