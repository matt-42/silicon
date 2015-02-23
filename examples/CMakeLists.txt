cmake_minimum_required(VERSION 2.8)

find_package( Iod )

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/.. ${IOD_INCLUDE_DIR})

add_definitions(-std=c++14  -ftemplate-depth=512 -O3 -DNDEBUG)

set(MHD_LIBS gnutls gcrypt pthread microhttpd)

add_executable(simple simple.cc)
target_link_libraries(simple ${MHD_LIBS})
