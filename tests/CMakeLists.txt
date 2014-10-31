cmake_minimum_required(VERSION 2.8)

enable_testing()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)
include_directories($ENV{HOME}/projects/iod)

add_definitions(-std=c++14 -g)

add_executable(test_backend test_backend.cc)
#add_test(test_backend test_backend)
