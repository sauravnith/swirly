# The Restful Matching-Engine.
# Copyright (C) 2013, 2017 Swirly Cloud Limited.
#
# This program is free software; you can redistribute it and/or modify it under the terms of the
# GNU General Public License as published by the Free Software Foundation; either version 2 of the
# License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with this program; if
# not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301, USA.

if(RAGEL_EXECUTABLE)
  add_custom_command(
    OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/RestBody.cpp"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMAND "${RAGEL_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/RestBody.rl"
            -o "${CMAKE_CURRENT_SOURCE_DIR}/RestBody.cpp"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/RestBody.rl")
endif(RAGEL_EXECUTABLE)

set(ws_SOURCES
  EntitySet.cpp
  Exception.cpp
  HttpHandler.cpp
  Page.cpp
  RestBody.cpp
  Rest.cpp
  Url.cpp
  http_parser.c)

add_library(ws_static STATIC ${ws_SOURCES})
set_target_properties(ws_static PROPERTIES OUTPUT_NAME swirly_ws)
target_link_libraries(ws_static clob_static)
install(TARGETS ws_static DESTINATION lib)

if(ENABLE_SHARED_LIBS)
  add_library(ws_shared SHARED ${ws_SOURCES})
  set_target_properties(ws_shared PROPERTIES OUTPUT_NAME swirly_ws)
  target_link_libraries(ws_shared clob_shared)
  install(TARGETS ws_shared DESTINATION lib)
endif()

set(ws_test_SOURCES
  EntitySetTest.cxx
  HttpHandlerTest.cxx
  PageTest.cxx
  RestBodyTest.cxx
  UrlTest.cxx)

foreach(file ${ws_test_SOURCES})
  get_filename_component (name ${file} NAME_WE)
  set(name "ws${name}")
  add_executable(${name} ${file})
  target_link_libraries(${name} ${ws_LIBRARY} ${unit_LIBRARY})
  add_test(NAME ${name} COMMAND ${name})
endforeach()
