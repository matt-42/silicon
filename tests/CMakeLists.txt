cmake_minimum_required(VERSION 2.8)

enable_testing()

find_package( OpenSSL )
find_package( Threads )

if (OPENSSL_FOUND)
    add_definitions(-DBOOST_NETWORK_ENABLE_HTTPS)
    include_directories(${OPENSSL_INCLUDE_DIR})
endif()

if (OPENSSL_FOUND)
  include_directories(${OPENSSL_INCLUDE_DIR})
endif (OPENSSL_FOUND)

find_package( Boost 1.54.0
    REQUIRED unit_test_framework system regex date_time thread filesystem
    program_options chrono atomic )

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)
include_directories($ENV{HOME}/projects/iod
                    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/mimosa/
                    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/install/include
                    )
                       

link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../third_party/install/lib
        ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/install/lib64)
#add_definitions(-std=c++14 -O3 -march=native)
add_definitions(-std=c++14 -Wall)
add_executable(test_backend test_backend.cc)
add_executable(sqlite sqlite.cc)
add_executable(middlewares middlewares.cc)

target_link_libraries(sqlite sqlite3 mimosa-http mimosa-uri mimosa gnutls z pthread lzma gnutls gcrypt pthread microhttpd)

target_link_libraries(middlewares sqlite3 mimosa-http mimosa-uri mimosa gnutls z pthread lzma gnutls gcrypt pthread microhttpd)

#add_test(test_backend test_backend)

add_executable(test_mimosa mimosa.cc)
target_link_libraries(test_mimosa mimosa-http mimosa-uri mimosa gnutls z pthread lzma)

add_executable(session session.cc)
target_link_libraries(session sqlite3 mimosa-http mimosa-uri mimosa gnutls z pthread lzma)

set(BOOST_SERVER_LIBS
    ${Boost_THREAD_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
    ${Boost_DATE_TIME_LIBRARY}
    ${Boost_PROGRAM_OPTIONS_LIBRARY}
    ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_CHRONO_LIBRARY})

add_executable(test_cppnet cppnetlib.cc)
target_link_libraries(test_cppnet ${BOOST_SERVER_LIBS} ${CMAKE_THREAD_LIBS_INIT} pthread
                                  cppnetlib-uri cppnetlib-client-connections cppnetlib-server-parsers ${OPENSSL_LIBRARIES})


add_executable(test_mdh mdh.cc)
target_link_libraries(test_mdh gnutls gcrypt pthread microhttpd)
