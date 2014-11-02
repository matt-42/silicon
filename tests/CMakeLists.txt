cmake_minimum_required(VERSION 2.8)

enable_testing()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)
include_directories($ENV{HOME}/projects/iod
                    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/mimosa)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../third_party/install/lib)
add_definitions(-std=c++14 -O3 -march=native)

add_executable(test_backend test_backend.cc)
#add_test(test_backend test_backend)

add_executable(test_mimosa mimosa.cc)
target_link_libraries(test_mimosa mimosa-http mimosa-uri mimosa gnutls z pthread lzma)
#add_mimosa(mimosa_backend mimosa_backend)
