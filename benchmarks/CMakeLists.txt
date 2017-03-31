cmake_minimum_required(VERSION 2.8)
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
                      ${CMAKE_SOURCE_DIR}/../cmake)

enable_testing()

set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -DNDEBUG -O3 -march=native -ftemplate-depth=1024 -g")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/.. $ENV{HOME}/local/include)
link_directories($ENV{HOME}/local/lib)

add_executable(urldecode urldecode.cc)
target_link_libraries(urldecode microhttpd benchmark pthread)
