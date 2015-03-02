cmake_minimum_required(VERSION 2.8)

find_package( Iod )

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/.. ${IOD_INCLUDE_DIR})

link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../third_party/install/lib
                ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/install/lib64
                $ENV{HOME}/local/lib)

add_definitions(-std=c++14  -ftemplate-depth=512)

set(MHD_LIBS gnutls gcrypt pthread microhttpd)
set(MIMOSA_LIBS mimosa-http mimosa-uri mimosa gnutls z pthread lzma)

add_executable(simple simple.cc)
target_link_libraries(simple ${MHD_LIBS})

add_executable(blog blog.cc)
target_link_libraries(blog ${MHD_LIBS} sqlite3 nettle)

add_executable(blog_test blog_test.cc)
target_link_libraries(blog_test sqlite3 nettle ${MIMOSA_LIBS} ${MHD_LIBS})
