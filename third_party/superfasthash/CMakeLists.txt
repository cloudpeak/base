# Copyright 2024 The Chromium Authors
# CMake build for superfasthash (Paul Hsieh's hash).

add_library(superfasthash STATIC
  superfasthash.c
)

target_include_directories(superfasthash PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CHROMIUM_SRC}
)

set_target_properties(superfasthash PROPERTIES
  POSITION_INDEPENDENT_CODE ON
  C_STANDARD 99
)

# This is third-party C code; suppress the noisy warnings.
# -w works with GCC/Clang (including clang-cl); pure MSVC uses /w.
if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
  target_compile_options(superfasthash PRIVATE -w)
elseif(MSVC)
  target_compile_options(superfasthash PRIVATE /w)
endif()
