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
                    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/websocketpp/
                    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/install/include
                    ${IOD_INCLUDE_DIR})

link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../third_party/install/lib
                ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/install/lib64
                $ENV{HOME}/local/lib)
#add_definitions(-std=c++14  -ftemplate-depth=1024 -O3 -DNDEBUG)
add_definitions(-std=c++14 -ftemplate-depth=1024 -g)
set(CPPNET_LIBS
    ${Boost_LIBRARIES}
    cppnetlib-uri cppnetlib-client-connections
    cppnetlib-server-parsers ${OPENSSL_LIBRARIES} pthread)

set(WEBSOCKETS_LIBS
    ${Boost_LIBRARIES} pthread)

message(STATUS ${WEBSOCKETS_LIBS})
set(MIMOSA_LIBS mimosa-http mimosa-uri mimosa gnutls z pthread lzma)

set(MHD_LIBS gnutls gcrypt pthread microhttpd)

# add_executable(cppnet_hello_world cppnet_hello_world.cc)
# target_link_libraries(cppnet_hello_world ${CPPNET_LIBS})

add_executable(sql_authentication sql_authentication.cc)
target_link_libraries(sql_authentication sqlite3 ${MHD_LIBS} curl)

add_executable(user_tracking user_tracking.cc)
target_link_libraries(user_tracking  ${MHD_LIBS} curl)

add_executable(user_tracking_mhd user_tracking_mhd.cc)
target_link_libraries(user_tracking_mhd  ${MHD_LIBS} curl)

add_executable(crud crud.cc)
target_link_libraries(crud sqlite3 ${MHD_LIBS} curl)

add_executable(libcurl_client libcurl_client.cc)
target_link_libraries(libcurl_client  ${MHD_LIBS} curl)

# add_executable(mimosa_hello_world mimosa_hello_world.cc)
# target_link_libraries(mimosa_hello_world ${MIMOSA_LIBS})

add_executable(microhttpd_hello_world microhttpd_hello_world.cc)
target_link_libraries(microhttpd_hello_world ${MHD_LIBS})

add_executable(sqlite sqlite.cc)
target_link_libraries(sqlite ${MHD_LIBS} sqlite3)

add_executable(mysql mysql.cc)
target_link_libraries(mysql ${MHD_LIBS} mysqlclient)

# add_executable(javascript_client javascript_client.cc)
# target_link_libraries(javascript_client ${MIMOSA_LIBS})

# add_executable(ws_echo_server ws_echo_server.cc)
# target_link_libraries(ws_echo_server ${WEBSOCKETS_LIBS})

# add_executable(ws_chat_server ws_chat_server.cc)
# target_link_libraries(ws_chat_server ${WEBSOCKETS_LIBS})

add_executable(rest rest.cc)
target_link_libraries(rest sqlite3 ${MHD_LIBS} curl)

add_executable(global_middlewares global_middlewares.cc)
target_link_libraries(global_middlewares ${MHD_LIBS} curl)

add_executable(api_nesting api_nesting.cc)
target_link_libraries(api_nesting ${MHD_LIBS} curl)

add_test(crud crud)
add_test(user_tracking user_tracking)
add_test(sql_authentication sql_authentication)
add_test(mysql mysql)
