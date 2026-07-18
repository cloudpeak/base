# Copyright 2024 The Chromium Authors
# CMake build for symbolize (Google glog symbolization subset).

add_library(symbolize STATIC
  demangle.cc
  symbolize.cc
)

target_include_directories(symbolize PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/..
)

target_compile_definitions(symbolize PUBLIC
  GLOG_EXPORT=
)

set_target_properties(symbolize PROPERTIES
  POSITION_INDEPENDENT_CODE ON
)

# Symbolization is slow unoptimized; mirror GN and force -O2 even in Debug.
target_compile_options(symbolize PRIVATE -O2)
