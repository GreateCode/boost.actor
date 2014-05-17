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
 * Dominik Charousset <dominik.charousset@haw-hamburg.de>                     *
 *                                                                            *
 * Distributed under the Boost Software License, Version 1.0. See             *
 * accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt  *
\******************************************************************************/


#include <vector>
#include <typeinfo>

#include "boost/actor/detail/memory.hpp"
#include "boost/actor/mailbox_element.hpp"

using namespace std;

#ifdef BOOST_ACTOR_DISABLE_MEM_MANAGEMENT

int cppa_memory_keep_compiler_happy() { return 0; }

#else // BOOST_ACTOR_DISABLE_MEM_MANAGEMENT

namespace boost {
namespace actor {
namespace detail {

namespace {

pthread_key_t s_key;
pthread_once_t s_key_once = PTHREAD_ONCE_INIT;

} // namespace <anonymous>

memory_cache::~memory_cache() { }

typedef map<const type_info*, unique_ptr<memory_cache> > cache_map;

void cache_map_destructor(void* ptr) {
    if (ptr) delete reinterpret_cast<cache_map*>(ptr);
}

void make_cache_map() {
    pthread_key_create(&s_key, cache_map_destructor);
}

cache_map& get_cache_map() {
    pthread_once(&s_key_once, make_cache_map);
    auto cache = reinterpret_cast<cache_map*>(pthread_getspecific(s_key));
    if (!cache) {
        cache = new cache_map;
        pthread_setspecific(s_key, cache);
        // insert default types
        unique_ptr<memory_cache> tmp(new basic_memory_cache<mailbox_element>);
        cache->insert(make_pair(&typeid(mailbox_element), move(tmp)));
    }
    return *cache;
}

memory_cache* memory::get_cache_map_entry(const type_info* tinf) {
    auto& cache = get_cache_map();
    auto i = cache.find(tinf);
    if (i != cache.end()) return i->second.get();
    return nullptr;
}

void memory::add_cache_map_entry(const type_info* tinf, memory_cache* instance) {
    auto& cache = get_cache_map();
    cache[tinf].reset(instance);
}

instance_wrapper::~instance_wrapper() { }

} } // namespace actor
} // namespace boost::detail

#endif // BOOST_ACTOR_DISABLE_MEM_MANAGEMENT
