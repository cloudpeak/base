# Copyright 2024 The Chromium Authors
# CMake build for dynamic_annotations.

add_library(dynamic_annotations STATIC
  dynamic_annotations.c
)

target_include_directories(dynamic_annotations PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/../valgrind
  ${CHROMIUM_SRC}
)

set_target_properties(dynamic_annotations PROPERTIES
  POSITION_INDEPENDENT_CODE ON
)
