# Copyright 2024 The Chromium Authors
# CMake build for symbolize (Google glog symbolization subset).

add_library(symbolize STATIC
  demangle.cc
  symbolize.cc
)

target_include_directories(symbolize PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CHROMIUM_SRC}
)

target_compile_definitions(symbolize PUBLIC
  GLOG_EXPORT=
)

set_target_properties(symbolize PROPERTIES
  POSITION_INDEPENDENT_CODE ON
)

# Symbolization is slow unoptimized; mirror GN and force -O2 even in Debug.
# clang-cl on Windows accepts -O2, but pure MSVC uses /O2.
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  target_compile_options(symbolize PRIVATE -O2)
elseif(MSVC)
  target_compile_options(symbolize PRIVATE /O2)
endif()
