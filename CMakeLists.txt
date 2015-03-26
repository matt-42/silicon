IF(BIICODE)

#
# biicode rules.
#


  file(READ ${CMAKE_SOURCE_DIR}/client_templates/javascript.js JAVASCRIPT_CLIENT_TEMPLATE)
  configure_file(${CMAKE_SOURCE_DIR}/silicon/clients/templates/javascript.hh.in
                 ${CMAKE_BINARY   _DIR}/silicon/clients/templates/javascript.hh)

  file(READ ${CMAKE_SOURCE_DIR}/client_templates/websocket.js JAVASCRIPT_WS_CLIENT_TEMPLATE)
  configure_file(${CMAKE_SOURCE_DIR}/silicon/clients/templates/javascript_websocket.hh.in
                 ${CMAKE_BINARY_DIR}/silicon/clients/templates/javascript_websocket.hh)

  ADD_BIICODE_TARGETS()

  target_include_directories(${BII_LIB_TARGET} PUBLIC ${CMAKE_CURRENT_BINARY_DIR})


  IF(APPLE)
     TARGET_COMPILE_OPTIONS(${BII_BLOCK_TARGET} INTERFACE "-std=c++14 -stdlib=libc++")
  ELSEIF (WIN32 OR UNIX)
     TARGET_COMPILE_OPTIONS(${BII_BLOCK_TARGET} INTERFACE "-std=c++14")
  ENDIF(APPLE)

ELSE()

#
# Compilation without biicode.
#

cmake_minimum_required(VERSION 2.8)

project(silicon)

file(READ ${CMAKE_SOURCE_DIR}/client_templates/javascript.js JAVASCRIPT_CLIENT_TEMPLATE)
configure_file(${CMAKE_SOURCE_DIR}/silicon/clients/templates/javascript.hh.in
               ${CMAKE_SOURCE_DIR}/silicon/clients/templates/javascript.hh)

file(READ ${CMAKE_SOURCE_DIR}/client_templates/websocket.js JAVASCRIPT_WS_CLIENT_TEMPLATE)
configure_file(${CMAKE_SOURCE_DIR}/silicon/clients/templates/javascript_websocket.hh.in
               ${CMAKE_SOURCE_DIR}/silicon/clients/templates/javascript_websocket.hh)

#export(PACKAGE Silicon)

#set(SILICON_INCLUDE_DIR "include")
#set(CMAKE_CONFIG_DEST "share/silicon")

#include(CMakePackageConfigHelpers)
#configure_package_config_file (                         
#        ${CMAKE_SOURCE_DIR}/SiliconConfig.cmake.in
#        ${CMAKE_BINARY_DIR}/SiliconConfig.cmake
#        INSTALL_DESTINATION ${CMAKE_CONFIG_DEST}
#        PATH_VARS SILICON_INCLUDE_DIR CMAKE_CONFIG_DEST)

#configure_file(SiliconConfigVersion.cmake.in
#  "${PROJECT_BINARY_DIR}/SiliconConfigVersion.cmake" @ONLY)

#install(FILES
#  "${PROJECT_BINARY_DIR}/SiliconConfig.cmake"
#  "${PROJECT_BINARY_DIR}/SiliconConfigVersion.cmake"
#  DESTINATION share/silicon)

install(DIRECTORY silicon DESTINATION include
  FILES_MATCHING PATTERN "*.hh")
install(DIRECTORY silicon DESTINATION include
  FILES_MATCHING PATTERN "*.hpp")


ENDIF() # BIICODE
