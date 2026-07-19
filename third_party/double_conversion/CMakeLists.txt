# Copyright 2024 The Chromium Authors
# CMake build for double_conversion.

add_library(double_conversion STATIC
  double-conversion/bignum-dtoa.cc
  double-conversion/bignum.cc
  double-conversion/cached-powers.cc
  double-conversion/double-to-string.cc
  double-conversion/fast-dtoa.cc
  double-conversion/fixed-dtoa.cc
  double-conversion/string-to-double.cc
  double-conversion/strtod.cc
)

target_include_directories(double_conversion PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/double-conversion
  ${CHROMIUM_SRC}
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  target_compile_options(double_conversion PRIVATE
    -Wno-unused-const-variable
    -Wno-unused-function
  )
endif()

set_target_properties(double_conversion PROPERTIES
  POSITION_INDEPENDENT_CODE ON
)
