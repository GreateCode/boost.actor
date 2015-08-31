cmake_minimum_required(VERSION 2.8)
project(boost_actor_io C CXX)

# get header files; only needed by CMake generators,
# e.g., for creating proper Xcode projects
file(GLOB_RECURSE LIBBOOST_ACTOR_IO_HDRS "caf/*.hpp")

# list cpp files excluding platform-dependent files
set (LIBBOOST_ACTOR_IO_SRCS
     src/basp.cpp
     src/basp_broker.cpp
     src/abstract_broker.cpp
     src/broker.cpp
     src/asio_multiplexer.cpp
     src/doorman.cpp
     src/max_msg_size.cpp
     src/middleman.cpp
     src/hook.cpp
     src/interfaces.cpp
     src/publish.cpp
     src/publish_local_groups.cpp
     src/remote_actor.cpp
     src/remote_group.cpp
     src/manager.cpp
     src/set_middleman.cpp
     src/scribe.cpp
     src/stream_manager.cpp
     src/test_multiplexer.cpp
     src/unpublish.cpp
     src/acceptor_manager.cpp
     src/multiplexer.cpp)

add_custom_target(libboost_actor_io)

# build shared library if not compiling static only
if (NOT BOOST_ACTOR_BUILD_STATIC_ONLY)
  add_library(libboost_actor_io_shared SHARED ${LIBBOOST_ACTOR_IO_SRCS} ${LIBBOOST_ACTOR_IO_HDRS})
  target_link_libraries(libboost_actor_io_shared ${LD_FLAGS} ${LIBBOOST_ACTOR_CORE_LIBRARY})
  set_target_properties(libboost_actor_io_shared
                        PROPERTIES
                        SOVERSION ${BOOST_ACTOR_VERSION}
                        VERSION ${BOOST_ACTOR_VERSION}
                        OUTPUT_NAME boost_actor_io)
  if(NOT WIN32)
    install(TARGETS libboost_actor_io_shared LIBRARY DESTINATION lib)
  endif()
  add_dependencies(libboost_actor_io_shared libboost_actor_io)
endif ()

# build static library only if --build-static or --build-static-only was set
if (BOOST_ACTOR_BUILD_STATIC_ONLY OR BOOST_ACTOR_BUILD_STATIC)
  add_library(libboost_actor_io_static STATIC ${LIBBOOST_ACTOR_IO_HDRS} ${LIBBOOST_ACTOR_IO_SRCS})
  target_link_libraries(libboost_actor_io_static ${LD_FLAGS} ${LIBBOOST_ACTOR_CORE_LIBRARY})
  set_target_properties(libboost_actor_io_static PROPERTIES OUTPUT_NAME boost_actor_io_static)
  if(NOT WIN32)
    install(TARGETS libboost_actor_io_static ARCHIVE DESTINATION lib)
  endif()
  add_dependencies(libboost_actor_io_static libboost_actor_io)
endif ()

link_directories(${LD_DIRS})
include_directories(. ${INCLUDE_DIRS})

# install includes
if(NOT WIN32)
  install(DIRECTORY caf/ DESTINATION include/caf FILES_MATCHING PATTERN "*.hpp")
endif()

