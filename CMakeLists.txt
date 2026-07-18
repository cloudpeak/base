# Copyright 2024 The Chromium Authors
# CMake build for the Chromium //base module — minimal core configuration.
#
# This is a minimal CMake-based build of //base targeting Linux + clang.
# Out of scope for this initial version:
#   - i18n (ICU)
#   - tracing (perfetto)
#   - rust
#   - PartitionAlloc-Everywhere (USE_PARTITION_ALLOC=false; raw_ptr is
#     the no-op implementation)
#   - PartitionAlloc-as-malloc / allocator shim
#   - All non-Linux platform code
#
# The output is a single static library `libbase.a`.

cmake_minimum_required(VERSION 3.16)

project(base LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)

# Homebrew clang 22 ships with libc++. The system libstdc++ 11 headers
# (from GCC 11 on Ubuntu 22.04) don't survive clang 22. Force libc++.
# Also add the Homebrew lib dir to the linker search path so try_compile
# checks (e.g. abseil's C++17 probe) can find -lc++ / -lc++abi.
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++ -L/home/linuxbrew/.linuxbrew/lib")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -stdlib=libc++ -L/home/linuxbrew/.linuxbrew/lib")
  link_directories(/home/linuxbrew/.linuxbrew/lib)
endif()

# Default to the Homebrew clang++ on WSL. Override by passing
# -DCMAKE_CXX_COMPILER=... on first configure.
if(NOT CMAKE_CXX_COMPILER)
  if(EXISTS /home/linuxbrew/.linuxbrew/bin/clang++)
    set(CMAKE_CXX_COMPILER /home/linuxbrew/.linuxbrew/bin/clang++)
    set(CMAKE_C_COMPILER   /home/linuxbrew/.linuxbrew/bin/clang)
  endif()
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

option(BASE_BUILD_TESTS "Build base unit tests" OFF)

# ---------------------------------------------------------------------------
# Global include configuration
# ---------------------------------------------------------------------------
# Every target in this tree uses `#include "base/..."` style paths rooted
# at the parent of //base. Likewise `#include "build/..."`,
# `#include "partition_alloc/..."` etc.
get_filename_component(BASE_ROOT ${CMAKE_CURRENT_SOURCE_DIR} ABSOLUTE)
get_filename_component(CHROMIUM_SRC ${BASE_ROOT}/.. ABSOLUTE)

add_library(base_compile_config INTERFACE)
# Note: ${BASE_ROOT} is added as -iquote (quote-only) below, NOT as a
# regular -I. Otherwise glibc's `#include <features.h>` (issued from
# /usr/include/string.h via <bits/libc-header-start.h>) would resolve to
# //base/features.h instead of /usr/include/features.h, and the glibc
# macros __BEGIN_DECLS / __THROW / __nonnull would never get defined.
target_include_directories(base_compile_config INTERFACE
  ${CHROMIUM_SRC}                                       # base/, third_party/...
  ${BASE_ROOT}/allocator/partition_allocator/src        # partition_alloc/
)
# -iquote: only visible to `#include "..."` (not `#include <...>`).
# This lets base sources use `#include "build/build_config.h"` and
# `#include "features.h"` while leaving glibc's <features.h> alone.
target_compile_options(base_compile_config INTERFACE "-iquote${BASE_ROOT}")

# ---------------------------------------------------------------------------
# abseil-cpp (external, from //third_party/abseil-cpp)
# ---------------------------------------------------------------------------
set(ABSL_PROPAGATE_CXX_STD ON CACHE BOOL "" FORCE)
set(ABSL_BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(ABSL_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
# Hide the bazel-style copts file lookups behind the build dir.
if(EXISTS ${CHROMIUM_SRC}/third_party/abseil-cpp/CMakeLists.txt)
  add_subdirectory(
    ${CHROMIUM_SRC}/third_party/abseil-cpp
    ${CMAKE_BINARY_DIR}/abseil-cpp
    EXCLUDE_FROM_ALL
  )
else()
  message(FATAL_ERROR "abseil-cpp not found at ${CHROMIUM_SRC}/third_party/abseil-cpp")
endif()

# ---------------------------------------------------------------------------
# Third-party //base/third_party/* dependencies
# ---------------------------------------------------------------------------
add_subdirectory(third_party/modp_b64)
add_subdirectory(third_party/double_conversion)
add_subdirectory(third_party/dynamic_annotations)
add_subdirectory(third_party/libevent)
add_subdirectory(third_party/symbolize)
add_subdirectory(third_party/xdg_user_dirs)
add_subdirectory(third_party/superfasthash)

# ---------------------------------------------------------------------------
# raw_ptr / partition_alloc — minimal build (no-op raw_ptr impl)
# ---------------------------------------------------------------------------
# When USE_PARTITION_ALLOC=false the raw_ptr component shrinks to
# pointers/empty.cc (a placeholder to satisfy the linker on Windows)
# and instance_tracer.cc whose body is #if'd out when the instance tracer
# is disabled. We compile them here as part of libbase.
set(PA_SRC_DIR
  ${BASE_ROOT}/allocator/partition_allocator/src/partition_alloc)

add_library(pa_raw_ptr STATIC
  ${PA_SRC_DIR}/pointers/empty.cc
  ${PA_SRC_DIR}/pointers/instance_tracer.cc
)
target_link_libraries(pa_raw_ptr PUBLIC base_compile_config)
set_target_properties(pa_raw_ptr PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_compile_definitions(pa_raw_ptr PUBLIC IS_RAW_PTR_IMPL=1)

# ---------------------------------------------------------------------------
# Collect base sources
# ---------------------------------------------------------------------------
# Use file(GLOB) for ergonomics, then filter out platform-specific files,
# tests, fuzzers and optional features we don't build here.
file(GLOB_RECURSE BASE_ALL_CC CONFIGURE_DEPENDS
  ${BASE_ROOT}/*.cc
)

# Patterns whose matches are dropped from the base library. Substrings
# (case-sensitive) — a file is excluded if its path contains any of these.
set(_exclude_substrings
  # Test/fuzz/perf code
  "_unittest.cc"
  "_test.cc"
  "_fuzzer.cc"
  "_perftest.cc"
  "_nocompile.nc"
  "unittest_utils.cc"
  "/test/"
  # Build-time only scratch files; never part of libbase
  "/cmake/"
  # The CMake build output dir is inside //base; never compile its contents
  "/cmake-build/"
  # Per-platform sources we don't build
  "/android/"
  "/apple/"
  "/win/"
  "/mac/"
  "/ios/"
  "/fuchsia/"
  "/nacl/"
  # Root-level platform-specific files (e.g. base_paths_android.cc)
  "_android.cc"
  "_fuchsia.cc"
  "_mac.cc"
  "_macos.cc"
  "_ios.cc"
  "_apple.cc"
  "_win.cc"
  "_winrt.cc"
  "_chromeos.cc"
  "_cros.cc"
  "_aix.cc"
  "_solaris.cc"
  "_freebsd.cc"
  "_openbsd.cc"
  "_netbsd.cc"
  "_bsd.cc"
  # BoringSSL-backed crypto — we use the standalone _nacl.cc versions
  # (md5_nacl.cc / sha1_nacl.cc) which are self-contained C++ ports.
  "_boringssl.cc"
  # i18n pulls in ICU; not in minimal core
  "/i18n/"
  # Tracing is disabled — replace with stub
  "/trace_event/"
  "/tracing/"
  # Subprocess testing targets
  "check_example.cc"
  "check_is_test.cc"
  # partition_alloc's own large library — handled above
  "/allocator/partition_allocator/"
  # The allocator dispatcher tests live under base/allocator/dispatcher/testing
  "/allocator/dispatcher/testing/"
  # Sampling heap profiler depends on partition_alloc internals we skip
  "/sampling_heap_profiler/"
  # Layout tests
  "/ substring_set_matcher/"
  # Explicit blink-only
  "discardable_shared_memory.cc"
  "shared_memory_switch.cc"
  "histogram_shared_memory.cc"
  "process/kill.cc"
  "process/launch.cc"
  "process/memory.cc"
  "process/process_iterator.cc"
  "process/kill_posix.cc"
  # file_path_watcher requires blink
  "file_path_watcher.cc"
  # sync_socket requires blink
  "sync_socket.cc"
  # message_pump_glib requires glib
  "message_pump_glib.cc"
  # Profiler unwinder backends we don't ship
  "/profiler/"
  # Android/Linux time_android
  "time/time_android.cc"
  # Android Linux shared sources (when not on Android)
  "memory_pressure_listener_android.cc"
  "memory_purge_manager_android.cc"
  "power_monitor_device_source_android.cc"
  "power_monitor_device_source_chromeos.cc"
  "threading/platform_thread_cros.cc"
  "feature_visitor.h"
  # clang profiling gated by buildflag
  "test/clang_profiling.cc"
  "test/clang_profiling_android.cc"
  # Native unwinder (Android)
  "native_unwinder_android"
  # Cronet
  "process/launch_ios.cc"
  # Embedded perfetto test tools
  "trace_event/"
  # posix/sysctl (Apple/BSD only)
  "posix/sysctl.cc"
  # Stubs that conflict on Linux
  "process/memory_stubs.cc"
  "process/process_stubs.cc"
  "files/file_path_watcher_stub.cc"
  # macOS (FSEvents) and BSD/macOS (kqueue) file watchers — Linux uses inotify
  "file_path_watcher_fsevents.cc"
  "file_path_watcher_kqueue.cc"
  # macOS/BSD kqueue-based message pump — Linux uses libevent/epoll
  "message_pump_kqueue.cc"
  # power_monitor_device_source_stub lives in trace_event etc.
  "power_monitor_device_source_stub.cc"
  # macOS IOKit-based power source sampling (depends on <IOKit/IOKitLib.h>)
  "iopm_power_source_sampling_event_source.cc"
  # NaCl-specific (depends on <nacl/nacl_random.h>). Other *_nacl.cc files
  # (md5_nacl.cc, sha1_nacl.cc, os_compat_nacl.cc, ...) are self-contained
  # C++ ports that we DO want to build.
  "rand_util_nacl.cc"
  # sys_info_nacl.cc duplicates SysInfo::VMAllocationGranularity() which is
  # already provided by sys_info_posix.cc on Linux.
  "sys_info_nacl.cc"
  # Fallback Semaphore implementation used only when neither Win/iOS/POSIX/Fuchsia
  # is selected. Linux uses semaphore_posix.cc.
  "semaphore_default.cc"
  # Test helpers that depend on gmock (not pulled in for the minimal build).
  "test_task_factory.cc"
  "test_utils.cc"
  # ICU-backed time helpers — we don't link ICU in the minimal build.
  "time_exploded_icu.cc"
  # Required for partition_alloc only
  "allocator/miracle_parameter.cc"
  "allocator/partition_alloc_features.cc"
  "allocator/partition_alloc_support.cc"
  "allocator/early_zone_registration_apple.cc"
)

# Files whose basename matches the stub we explicitly keep (override the
# broad "/trace_event/" exclusion above).
set(_keep_files
  "${BASE_ROOT}/trace_event/trace_event_stub.cc"
  "${BASE_ROOT}/trace_event/trace_event_stub.h"
  "${BASE_ROOT}/files/file_path_watcher_stub.cc"
  # Stubs for partition_alloc symbols (e.g. TerminateBecauseOutOfMemory)
  # referenced by //base when partition_alloc proper is not built.
  "${BASE_ROOT}/cmake/partition_alloc_stubs.cc"
)

set(BASE_CC)
foreach(f ${BASE_ALL_CC})
  set(exclude FALSE)
  foreach(p ${_exclude_substrings})
    if(f MATCHES "${p}")
      set(exclude TRUE)
      break()
    endif()
  endforeach()
  if(NOT exclude)
    list(APPEND BASE_CC ${f})
  endif()
endforeach()
# Force-keep the stub sources even if excluded above.
foreach(k ${_keep_files})
  if(EXISTS ${k})
    list(APPEND BASE_CC ${k})
    list(REMOVE_DUPLICATES BASE_CC)
  endif()
endforeach()

list(LENGTH BASE_CC _n)
message(STATUS "base: ${_n} .cc sources will be compiled into libbase")

# ---------------------------------------------------------------------------
# libbase
# ---------------------------------------------------------------------------
add_library(base STATIC ${BASE_CC})
set_target_properties(base PROPERTIES
  POSITION_INDEPENDENT_CODE ON
  OUTPUT_NAME base
)

target_link_libraries(base
  PUBLIC
    base_compile_config
    pa_raw_ptr
    modp_b64
    double_conversion
    dynamic_annotations
    libevent
    symbolize
    xdg_user_dirs
    superfasthash
    # abseil-cpp — link individual components. Header-only absl targets
    # are pulled in transitively by absl::base / absl::core_headers.
    absl::base
    absl::core_headers
    absl::strings
    absl::int128
    absl::numeric
    absl::optional
    absl::variant
    absl::any
    absl::span
    absl::fixed_array
    absl::flat_hash_map
    absl::flat_hash_set
    absl::memory
    absl::synchronization
    absl::time
    absl::hash
    absl::city
    absl::function_ref
    absl::any_invocable
    absl::utility
    absl::meta
    absl::cleanup
    absl::debugging
    absl::log_severity
    absl::no_destructor
    absl::nullability
  PRIVATE
    Threads::Threads
    ${CMAKE_DL_LIBS}
    rt
)

# Required for symbolize (line numbers from DWARF) on Linux.
target_compile_definitions(base
  PRIVATE
    BASE_IMPLEMENTATION=1
    IS_RAW_PTR_IMPL=1
    USE_SYMBOLIZE
    SYSTEM_NATIVE_UTF8
    BASE_FEATURES_IMPLEMENTATION
    LOGGING_IMPLEMENTATION
    VLOG_IMPLEMENTATION
    BASE_FILE_PATH_WATCHER_IMPLEMENTATION
    BASE_MEMORY_PRESSURE_LEVEL_REPORTING_IMPLEMENTATION
)

target_compile_options(base PRIVATE
  -Wno-unknown-warning-option
  -Wno-unused-function
  -Wno-unused-variable
  -Wno-unused-but-set-variable
  -Wno-deprecated-declarations
  -Wno-deprecated
  -Wno-implicit-fallthrough
  -Wno-unused-private-field
  -Wno-shadow
  -Wno-shorten-64-to-32
  -Wno-sign-compare
  -Wno-undefined-func-template
  -Wno-unused-template
  -Wno-weak-vtables
  -Wno-format-pedantic
  -Wno-zero-as-null-pointer-constant
  -Wno-non-c-typedef-for-linkage
  -Wno-unused-const-variable
  -Wno-missing-field-initializers
  -Wno-unused-macros
  -Wno-old-style-cast
  -Wno-unreachable-code
  -Wno-unreachable-code-return
  -Wno-unreachable-code-break
  -Wno-unused-local-typedef
  -Wno-ctad-maybe-unsupported
  -Wno-reserved-identifier
  -Wno-unused-member-function
  -Wno-undefined-internal-using-type
  -Wno-double-promotion
  -Wno-pedantic
  -Wno-c++20-designator
  -Wno-nontrivial-memcall
)

# Generated headers we created in source dirs (buildflag files) are
# reachable via base_compile_config (BASE_ROOT is on the include path).

# ---------------------------------------------------------------------------
# Optional test executable: just a smoke check that libbase links.
# ---------------------------------------------------------------------------
if(BASE_BUILD_TESTS)
  add_executable(base_smoke_test
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/smoke_test_main.cc
  )
  target_link_libraries(base_smoke_test PRIVATE base)
endif()
