cmake_minimum_required(VERSION 2.8)

project(silicon)

file(READ ${CMAKE_SOURCE_DIR}/client_templates/javascript.js JAVASCRIPT_CLIENT_TEMPLATE)

configure_file(${CMAKE_SOURCE_DIR}/silicon/client_templates/javascript.hh.in
               ${CMAKE_SOURCE_DIR}/silicon/client_templates/javascript.hh)
