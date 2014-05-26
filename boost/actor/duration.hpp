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


#ifndef BOOST_ACTOR_DURATION_HPP
#define BOOST_ACTOR_DURATION_HPP

#include <string>
#include <chrono>
#include <cstdint>
#include <stdexcept>

namespace boost {
namespace actor {

/**
 * @brief SI time units to specify timeouts.
 */
enum class time_unit : std::uint32_t {
    invalid = 0,
    seconds = 1,
    milliseconds = 1000,
    microseconds = 1000000
};

// minutes are implicitly converted to seconds
template<std::intmax_t Num, std::intmax_t Denom>
struct ratio_to_time_unit_helper {
    static constexpr time_unit value = time_unit::invalid;
};

template<> struct ratio_to_time_unit_helper<1, 1> {
    static constexpr time_unit value = time_unit::seconds;
};

template<> struct ratio_to_time_unit_helper<1, 1000> {
    static constexpr time_unit value = time_unit::milliseconds;
};

template<> struct ratio_to_time_unit_helper<1, 1000000> {
    static constexpr time_unit value = time_unit::microseconds;
};

template<> struct ratio_to_time_unit_helper<60, 1> {
    static constexpr time_unit value = time_unit::seconds;
};

/**
 * @brief Converts an STL time period to a
 *        {@link boost::actor::detail::time_unit time_unit}.
 */
template<typename Period>
constexpr time_unit get_time_unit_from_period() {
    return ratio_to_time_unit_helper<Period::num, Period::den>::value;
}

/**
 * @brief Time duration consisting of a {@link boost::actor::detail::time_unit time_unit}
 *        and a 64 bit unsigned integer.
 */
class duration {

 public:

    constexpr duration() : unit(time_unit::invalid), count(0) { }

    constexpr duration(time_unit u, std::uint32_t v) : unit(u), count(v) { }

    /**
     * @brief Creates a new instance from an STL duration.
     * @throws std::invalid_argument Thrown if <tt>d.count()</tt> is negative.
     */
    template<class Rep, class Period>
    duration(std::chrono::duration<Rep, Period> d)
    : unit(get_time_unit_from_period<Period>()), count(rd(d)) {
        static_assert(get_time_unit_from_period<Period>() != time_unit::invalid,
                      "boost::actor::duration supports only minutes, seconds, "
                      "milliseconds or microseconds");
    }

    /**
     * @brief Creates a new instance from an STL duration given in minutes.
     * @throws std::invalid_argument Thrown if <tt>d.count()</tt> is negative.
     */
    template<class Rep>
    duration(std::chrono::duration<Rep, std::ratio<60, 1> > d)
    : unit(time_unit::seconds), count(rd(d) * 60) { }

    /**
     * @brief Returns true if <tt>unit != time_unit::none</tt>.
     */
    inline bool valid() const { return unit != time_unit::invalid; }

    /**
     * @brief Returns true if <tt>count == 0</tt>.
     */
    inline bool is_zero() const { return count == 0; }

    std::string to_string() const;

    time_unit unit;

    std::uint64_t count;

 private:

    // reads d.count and throws invalid_argument if d.count < 0
    template<class Rep, class Period>
    static inline std::uint64_t
    rd(const std::chrono::duration<Rep, Period>& d) {
        if (d.count() < 0) {
            throw std::invalid_argument("negative durations are not supported");
        }
        return static_cast<std::uint64_t>(d.count());
    }

    template<class Rep>
    static inline std::uint64_t
    rd(const std::chrono::duration<Rep, std::ratio<60, 1>>& d) {
        // convert minutes to seconds on-the-fly
        return rd(std::chrono::duration<Rep, std::ratio<1, 1>>(d.count()) * 60);
    }


};

/**
 * @relates duration
 */
bool operator==(const duration& lhs, const duration& rhs);

/**
 * @relates duration
 */
inline bool operator!=(const duration& lhs, const duration& rhs) {
    return !(lhs == rhs);
}

} // namespace actor
} // namespace boost

/**
 * @relates boost::actor::duration
 */
template<class Clock, class Duration>
std::chrono::time_point<Clock, Duration>&
operator+=(std::chrono::time_point<Clock, Duration>& lhs,
           const boost::actor::duration& rhs) {
    switch (rhs.unit) {
        case boost::actor::time_unit::seconds:
            lhs += std::chrono::seconds(rhs.count);
            break;

        case boost::actor::time_unit::milliseconds:
            lhs += std::chrono::milliseconds(rhs.count);
            break;

        case boost::actor::time_unit::microseconds:
            lhs += std::chrono::microseconds(rhs.count);
            break;

        case boost::actor::time_unit::invalid:
            break;
    }
    return lhs;
}

#endif // BOOST_ACTOR_DURATION_HPP
