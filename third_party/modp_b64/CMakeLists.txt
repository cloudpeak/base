# Copyright 2024 The Chromium Authors
# CMake build for modp_b64.

add_library(modp_b64 STATIC
  modp_b64.cc
)

# modp_b64.cc includes "base/third_party/modp_b64/modp_b64.h" — the
# chromium-style path needs //base's parent on the include path.
target_include_directories(modp_b64 PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/..
)

set_target_properties(modp_b64 PROPERTIES
  POSITION_INDEPENDENT_CODE ON
)
