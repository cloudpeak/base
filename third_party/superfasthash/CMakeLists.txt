# Copyright 2024 The Chromium Authors
# CMake build for superfasthash (Paul Hsieh's hash).

add_library(superfasthash STATIC
  superfasthash.c
)

target_include_directories(superfasthash PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/..
)

set_target_properties(superfasthash PROPERTIES
  POSITION_INDEPENDENT_CODE ON
  C_STANDARD 99
)

# This is third-party C code; suppress the noisy warnings.
target_compile_options(superfasthash PRIVATE -w)
