# Copyright 2024 The Chromium Authors
# CMake build for modp_b64.

add_library(modp_b64 STATIC
  modp_b64.cc
)

# modp_b64.cc includes "base/third_party/modp_b64/modp_b64.h" — the
# chromium-style path needs //base's parent on the include path.
# CHROMIUM_SRC (defined by base/CMakeLists.txt as ${BASE_ROOT}/..) is the
# chromium-style root so `#include "base/third_party/modp_b64/modp_b64.h"`
# resolves. Using CMAKE_SOURCE_DIR/.. breaks when base is consumed as a
# subdirectory of another project (e.g. tin), because CMAKE_SOURCE_DIR is
# the top-level project dir, not base's parent.
target_include_directories(modp_b64 PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CHROMIUM_SRC}
)

set_target_properties(modp_b64 PROPERTIES
  POSITION_INDEPENDENT_CODE ON
)
