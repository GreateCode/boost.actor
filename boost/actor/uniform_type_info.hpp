/******************************************************************************\
 *           ___        __                                                    *
 *          /\_ \    __/\ \                                                   *
 *          \//\ \  /\_\ \ \____    ___   _____   _____      __               *
 *            \ \ \ \/\ \ \ '__`\  /'___\/\ '__`\/\ '__`\  /'__`\             *
 *             \_\ \_\ \ \ \ \L\ \/\ \__/\ \ \L\ \ \ \L\ \/\ \L\.\_           *
 *             /\____\\ \_\ \_,__/\ \____\\ \ ,__/\ \ ,__/\ \__/.\_\          *
 *             \/____/ \/_/\/___/  \/____/ \ \ \/  \ \ \/  \/__/\/_/          *
 *                                          \ \_\   \ \_\                     *
 *                                           \/_/    \/_/                     *
 *                                                                            *
 * Copyright (C) 2011-2013                                                    *
 * Dominik Charousset <dominik.charousset@haw-hamburg.de>                     *
 *                                                                            *
 * This file is part of libcppa.                                              *
 * libcppa is free software: you can redistribute it and/or modify it under   *
 * the terms of the GNU Lesser General Public License as published by the     *
 * Free Software Foundation; either version 2.1 of the License,               *
 * or (at your option) any later version.                                     *
 *                                                                            *
 * libcppa is distributed in the hope that it will be useful,                 *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                       *
 * See the GNU Lesser General Public License for more details.                *
 *                                                                            *
 * You should have received a copy of the GNU Lesser General Public License   *
 * along with libcppa. If not, see <http://www.gnu.org/licenses/>.            *
\******************************************************************************/


#ifndef BOOST_ACTOR_UNIFORM_TYPE_INFO_HPP
#define BOOST_ACTOR_UNIFORM_TYPE_INFO_HPP

#include <map>
#include <vector>
#include <memory>
#include <string>
#include <cstdint>
#include <typeinfo>
#include <type_traits>

#include "boost/actor/config.hpp"

#include "boost/actor/util/comparable.hpp"
#include "boost/actor/util/type_traits.hpp"

#include "boost/actor/detail/demangle.hpp"
#include "boost/actor/detail/to_uniform_name.hpp"

namespace boost {
namespace actor {

class serializer;
class deserializer;
class uniform_type_info;

const uniform_type_info* uniform_typeid(const std::type_info&);

struct uniform_value_t;

typedef std::unique_ptr<uniform_value_t> uniform_value;

struct uniform_value_t {
    const uniform_type_info* ti;
    void* val;
    virtual uniform_value copy() = 0;
    virtual ~uniform_value_t();
};

template<typename T, typename... Ts>
uniform_value make_uniform_value(const uniform_type_info* ti, Ts&&... args) {
    struct container : uniform_value_t {
        T value;
        container(const uniform_type_info* ptr, T arg) : value(std::move(arg)) {
            ti = ptr;
            val = &value;
        }
        uniform_value copy() override {
            return uniform_value{new container(ti, value)};
        }
    };
    return uniform_value{new container(ti, T(std::forward<Ts>(args)...))};
}

/**
 * @defgroup TypeSystem libcppa's platform-independent type system.
 *
 * @p libcppa provides a fully network transparent communication between
 * actors. Thus, @p libcppa needs to serialize and deserialize message objects.
 * Unfortunately, this is not possible using the C++ RTTI system.
 *
 * Since it is not possible to extend <tt>std::type_info</tt>, @p libcppa
 * uses its own type abstraction:
 * {@link cppa::uniform_type_info uniform_type_info}.
 *
 * Unlike <tt>std::type_info::name()</tt>,
 * {@link cppa::uniform_type_info::name() uniform_type_info::name()}
 * is guaranteed to return the same name on all supported platforms.
 * Furthermore, it allows to create an instance of a type by name:
 *
 * @code
 * // creates a signed, 32 bit integer
 * cppa::object i = cppa::uniform_type_info::by_name("@i32")->create();
 * @endcode
 *
 * However, you should rarely if ever need to use {@link cppa::object object}
 * or {@link cppa::uniform_type_info uniform_type_info}.
 *
 * There is one exception, though, where you need to care about
 * <tt>libcppa</tt>'s type system: using custom data types in messages.
 * The source code below compiles fine, but crashes with an exception during
 * runtime.
 *
 * @code
 * #include "boost/actor/cppa.hpp"
 * using namespace boost::actor;
 *
 * struct foo { int a; int b; };
 *
 * int main()
 * {
 *     send(self, foo{1, 2});
 *     return 0;
 * }
 * @endcode
 *
 * Depending on your platform, the error message looks somewhat like this:
 *
 * <tt>
 * terminate called after throwing an instance of std::runtime_error
 * <br>
 * what():  uniform_type_info::by_type_info(): foo is an unknown typeid name
 * </tt>
 *
 * The user-defined struct @p foo is not known by the @p libcppa type system.
 * Thus, @p libcppa is unable to serialize/deserialize @p foo and rejects it.
 *
 * Fortunately, there is an easy way to add @p foo the type system,
 * without needing to implement serialize/deserialize by yourself:
 *
 * @code
 * cppa::announce<foo>(&foo::a, &foo::b);
 * @endcode
 *
 * {@link cppa::announce announce()} takes the class as template
 * parameter and pointers to all members (or getter/setter pairs)
 * as arguments. This works for all primitive data types and STL compliant
 * containers. See the announce {@link announce_example_1.cpp example 1},
 * {@link announce_example_2.cpp example 2},
 * {@link announce_example_3.cpp example 3} and
 * {@link announce_example_4.cpp example 4} for more details.
 *
 * Obviously, there are limitations. If your class does implement
 * an unsupported data structure, you have to implement serialize/deserialize
 * by yourself. {@link announce_example_5.cpp Example 5} shows, how to
 * announce a tree data structure to the @p libcppa type system.
 */

/**
 * @ingroup TypeSystem
 * @brief Provides a platform independent type name and a (very primitive)
 *        kind of reflection in combination with {@link cppa::object object}.
 *
 * The platform independent name is equal to the "in-sourcecode-name"
 * with a few exceptions:
 * - @c std::string is named @c \@str
 * - @c std::u16string is named @c \@u16str
 * - @c std::u32string is named @c \@u32str
 * - @c integers are named <tt>\@(i|u)$size</tt>\n
 *   e.g.: @c \@i32 is a 32 bit signed integer; @c \@u16
 *   is a 16 bit unsigned integer
 * - the <em>anonymous namespace</em> is named @c \@_ \n
 *   e.g.: <tt>namespace { class foo { }; }</tt> is mapped to
 *   @c \@_::foo
 */
class uniform_type_info {

    friend bool operator==(const uniform_type_info& lhs,
                           const uniform_type_info& rhs);

    // disable copy and move constructors
    uniform_type_info(uniform_type_info&&) = delete;
    uniform_type_info(const uniform_type_info&) = delete;

    // disable assignment operators
    uniform_type_info& operator=(uniform_type_info&&) = delete;
    uniform_type_info& operator=(const uniform_type_info&) = delete;

 public:

    virtual ~uniform_type_info();

    /**
     * @brief Get instance by uniform name.
     * @param uniform_name The @p libcppa internal name for a type.
     * @returns The instance associated to @p uniform_name.
     * @throws std::runtime_error if no type named @p uniform_name was found.
     */
    static const uniform_type_info* from(const std::string& uniform_name);

    /**
     * @brief Get instance by std::type_info.
     * @param tinfo A STL RTTI object.
     * @returns An instance describing the same type as @p tinfo.
     * @throws std::runtime_error if @p tinfo is not an announced type.
     */
    static const uniform_type_info* from(const std::type_info& tinfo);

    /**
     * @brief Get all instances.
     * @returns A vector with all known (announced) instances.
     */
    static std::vector<const uniform_type_info*> instances();

    /**
     * @brief Creates a copy of @p other.
     */
    virtual uniform_value create(const uniform_value& other = uniform_value{}) const = 0;

    /**
     * @brief Deserializes an object of this type from @p source.
     */
    uniform_value deserialize(deserializer* source) const;

    /**
     * @brief Get the internal @p libcppa name for this type.
     * @returns A string describing the @p libcppa internal type name.
     */
    virtual const char* name() const = 0;

    /**
     * @brief Determines if this uniform_type_info describes the same
     *        type than @p tinfo.
     * @returns @p true if @p tinfo describes the same type as @p this.
     */
    virtual bool equal_to(const std::type_info& tinfo) const = 0;

    /**
     * @brief Compares two instances of this type.
     * @param instance1 Left hand operand.
     * @param instance2 Right hand operand.
     * @returns @p true if <tt>*instance1 == *instance2</tt>.
     * @pre @p instance1 and @p instance2 have the type of @p this.
     */
    virtual bool equals(const void* instance1, const void* instance2) const = 0;

    /**
     * @brief Serializes @p instance to @p sink.
     * @param instance Instance of this type.
     * @param sink Target data sink.
     * @pre @p instance has the type of @p this.
     */
    virtual void serialize(const void* instance, serializer* sink) const = 0;

    /**
     * @brief Deserializes @p instance from @p source.
     * @param instance Instance of this type.
     * @param source Data source.
     * @pre @p instance has the type of @p this.
     */
    virtual void deserialize(void* instance, deserializer* source) const = 0;

    /**
     * @brief Returns @p instance encapsulated as an @p any_tuple.
     */
    virtual any_tuple as_any_tuple(void* instance) const = 0;

 protected:

    uniform_type_info() = default;

    template<typename T>
    uniform_value create_impl(const uniform_value& other) const {
        if (other) {
            BOOST_ACTOR_REQUIRE(other->ti == this);
            auto ptr = reinterpret_cast<const T*>(other->val);
            return make_uniform_value<T>(this, *ptr);
        }
        return make_uniform_value<T>(this);
    }

};

/**
 * @relates uniform_type_info
 */
template<typename T>
inline const uniform_type_info* uniform_typeid() {
    return uniform_typeid(typeid(T));
}

/**
 * @relates uniform_type_info
 */
inline bool operator==(const uniform_type_info& lhs,
                       const uniform_type_info& rhs) {
    // uniform_type_info instances are singletons,
    // thus, equal == identical
    return &lhs == &rhs;
}

/**
 * @relates uniform_type_info
 */
inline bool operator!=(const uniform_type_info& lhs,
                       const uniform_type_info& rhs) {
    return !(lhs == rhs);
}

/**
 * @relates uniform_type_info
 */
inline bool operator==(const uniform_type_info& lhs, const std::type_info& rhs) {
    return lhs.equal_to(rhs);
}

/**
 * @relates uniform_type_info
 */
inline bool operator!=(const uniform_type_info& lhs, const std::type_info& rhs) {
    return !(lhs.equal_to(rhs));
}

/**
 * @relates uniform_type_info
 */
inline bool operator==(const std::type_info& lhs, const uniform_type_info& rhs) {
    return rhs.equal_to(lhs);
}

/**
 * @relates uniform_type_info
 */
inline bool operator!=(const std::type_info& lhs, const uniform_type_info& rhs) {
    return !(rhs.equal_to(lhs));
}

} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_UNIFORM_TYPE_INFO_HPP
