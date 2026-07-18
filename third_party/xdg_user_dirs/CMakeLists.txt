# Copyright 2024 The Chromium Authors
# CMake build for xdg_user_dirs.

add_library(xdg_user_dirs STATIC
  xdg_user_dir_lookup.cc
)

target_include_directories(xdg_user_dirs PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/..
)

set_target_properties(xdg_user_dirs PROPERTIES
  POSITION_INDEPENDENT_CODE ON
)
