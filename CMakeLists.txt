cmake_minimum_required(VERSION 2.8)

project(silicon)

file(READ ${CMAKE_SOURCE_DIR}/client_templates/javascript.js JAVASCRIPT_CLIENT_TEMPLATE)
configure_file(${CMAKE_SOURCE_DIR}/silicon/clients/templates/javascript.hh.in
               ${CMAKE_SOURCE_DIR}/silicon/clients/templates/javascript.hh)

file(READ ${CMAKE_SOURCE_DIR}/client_templates/websocket.js JAVASCRIPT_WS_CLIENT_TEMPLATE)
configure_file(${CMAKE_SOURCE_DIR}/silicon/clients/templates/javascript_websocket.hh.in
               ${CMAKE_SOURCE_DIR}/silicon/clients/templates/javascript_websocket.hh)

install(DIRECTORY silicon DESTINATION include
  FILES_MATCHING PATTERN "*.hh")
install(DIRECTORY silicon DESTINATION include
  FILES_MATCHING PATTERN "*.hpp")
