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

#include "boost/actor/detail/memory.hpp"

#include <vector>
#include <typeinfo>

#include "boost/actor/mailbox_element.hpp"

#ifdef BOOST_ACTOR_NO_MEM_MANAGEMENT

int caf_memory_keep_compiler_happy() {
  // this function shuts up a linker warning saying that the
  // object file has no symbols
  return 0;
}

#else // BOOST_ACTOR_NO_MEM_MANAGEMENT

namespace boost {
namespace actor {
namespace detail {

namespace {

using cache_map = std::map<const std::type_info*, std::unique_ptr<memory_cache>>;

} // namespace <anonymous>

#if defined(BOOST_ACTOR_CLANG) || defined(BOOST_ACTOR_MACOS)
namespace {

pthread_key_t s_key;
pthread_once_t s_key_once = PTHREAD_ONCE_INIT;

} // namespace <anonymous>

void cache_map_destructor(void* ptr) {
  delete reinterpret_cast<cache_map*>(ptr);
}

void make_cache_map() {
  pthread_key_create(&s_key, cache_map_destructor);
}

cache_map& get_cache_map() {
  pthread_once(&s_key_once, make_cache_map);
  auto cache = reinterpret_cast<cache_map*>(pthread_getspecific(s_key));
  if (! cache) {
    cache = new cache_map;
    pthread_setspecific(s_key, cache);
    // insert default types
    std::unique_ptr<memory_cache> tmp(new basic_memory_cache<mailbox_element>);
    cache->emplace(&typeid(mailbox_element), std::move(tmp));
  }
  return *cache;
}

#else // !BOOST_ACTOR_CLANG && !BOOST_ACTOR_MACOS

namespace {

thread_local std::unique_ptr<cache_map> s_key;

} // namespace <anonymous>

cache_map& get_cache_map() {
  if (! s_key) {
    s_key = std::unique_ptr<cache_map>(new cache_map);
    // insert default types
    std::unique_ptr<memory_cache> tmp(new basic_memory_cache<mailbox_element>);
    s_key->emplace(&typeid(mailbox_element), std::move(tmp));
  }
  return *s_key;
}

#endif

memory_cache::~memory_cache() {
  // nop
}

memory_cache* memory::get_cache_map_entry(const std::type_info* tinf) {
  auto& cache = get_cache_map();
  auto i = cache.find(tinf);
  if (i != cache.end()) {
    return i->second.get();
  }
  return nullptr;
}

void memory::add_cache_map_entry(const std::type_info* tinf,
                                 memory_cache* instance) {
  auto& cache = get_cache_map();
  cache[tinf].reset(instance);
}

} // namespace detail
} // namespace actor
} // namespace boost

#endif // BOOST_ACTOR_NO_MEM_MANAGEMENT
