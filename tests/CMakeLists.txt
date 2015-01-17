cmake_minimum_required(VERSION 2.8)

enable_testing()


find_package( OpenSSL )
find_package( Threads )
find_package( Iod )
find_package( Boost COMPONENTS system thread date_time program_options filesystem chrono)

include(${IOD_USE_FILE})

set(CMAKE_VERBOSE_MAKEFILE ON)
if (OPENSSL_FOUND)
    add_definitions(-DBOOST_NETWORK_ENABLE_HTTPS)
    include_directories(${OPENSSL_INCLUDE_DIR})
endif()

if (OPENSSL_FOUND)
  include_directories(${OPENSSL_INCLUDE_DIR})
endif (OPENSSL_FOUND)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../third_party/mimosa/
                    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/install/include
                    ${IOD_INCLUDE_DIR})

link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../third_party/install/lib
                ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/install/lib64
                $ENV{HOME}/local/lib)
add_definitions(-O3 -march=native -ftemplate-depth=512)
add_definitions(-std=c++14)

set(CPPNET_LIBS
    ${Boost_LIBRARIES}
    cppnetlib-uri cppnetlib-client-connections
    cppnetlib-server-parsers ${OPENSSL_LIBRARIES} pthread)

set(MIMOSA_LIBS mimosa-http mimosa-uri mimosa gnutls z pthread lzma)

set(MHD_LIBS gnutls gcrypt pthread microhttpd)

add_iod_executable(cppnet_hello_world cppnet_hello_world.cc)
target_link_libraries(cppnet_hello_world ${CPPNET_LIBS})

add_iod_executable(sql_authentication sql_authentication.cc)
target_link_libraries(sql_authentication sqlite3 ${MIMOSA_LIBS})

add_iod_executable(user_tracking user_tracking.cc)
target_link_libraries(user_tracking  ${MIMOSA_LIBS})


add_iod_executable(user_tracking_mhd user_tracking_mhd.cc)
target_link_libraries(user_tracking_mhd  ${MHD_LIBS} ${MIMOSA_LIBS})

add_iod_executable(crud crud.cc)
target_link_libraries(crud sqlite3 ${MIMOSA_LIBS})

add_iod_executable(client client.cc)
target_link_libraries(client sqlite3 ${MIMOSA_LIBS})

add_iod_executable(mimosa_hello_world mimosa_hello_world.cc)
target_link_libraries(mimosa_hello_world ${MIMOSA_LIBS})

add_iod_executable(microhttpd_hello_world microhttpd_hello_world.cc)
target_link_libraries(microhttpd_hello_world ${MHD_LIBS})


add_iod_executable(javascript_client javascript_client.cc)
target_link_libraries(javascript_client)

add_test(crud crud)
add_test(client client)
add_test(user_tracking user_tracking)
add_test(sql_authentication sql_authentication)
