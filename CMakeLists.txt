# Copyright 2024 The Chromium Authors
# CMake build for the Chromium //base module — minimal core configuration.
#
# This is a minimal CMake-based build of //base targeting Linux + clang and
# Windows + clang-cl (VS2022).
# Out of scope for this initial version:
#   - i18n (ICU)
#   - tracing (perfetto)
#   - rust
#   - PartitionAlloc-Everywhere (USE_PARTITION_ALLOC=false; raw_ptr is
#     the no-op implementation)
#   - PartitionAlloc-as-malloc / allocator shim
#
# The output is a single static library `libbase.a` (Linux) / `base.lib`
# (Windows).

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
# Guard with UNIX so this does NOT run on Windows clang-cl (which uses
# the MSVC standard library and has no /home/linuxbrew path).
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND UNIX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++ -L/home/linuxbrew/.linuxbrew/lib")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -stdlib=libc++ -L/home/linuxbrew/.linuxbrew/lib")
  link_directories(/home/linuxbrew/.linuxbrew/lib)
endif()

# Default to the Homebrew clang++ on WSL. Override by passing
# -DCMAKE_CXX_COMPILER=... on first configure.
# On Windows, the caller selects clang-cl via the VS toolset (-T ClangCL)
# or by passing -DCMAKE_CXX_COMPILER=clang-cl explicitly.
if(UNIX AND NOT CMAKE_CXX_COMPILER)
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
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/stubs               # testing/gtest/... stub
)
# -iquote: only visible to `#include "..."` (not `#include <...>`).
# This lets base sources use `#include "build/build_config.h"` and
# `#include "features.h"` while leaving glibc's <features.h> alone.
# On Windows there is no glibc <features.h> shadowing issue, so we can
# use a regular -I. clang-cl supports -iquote, but the MSVC generator
# may not handle it correctly in all contexts.
if(UNIX)
  target_compile_options(base_compile_config INTERFACE "-iquote${BASE_ROOT}")
else()
  target_include_directories(base_compile_config INTERFACE ${BASE_ROOT})
endif()

# Windows-specific compile definitions. These must be set on
# base_compile_config (an INTERFACE target) so they propagate to base
# and all of its third-party dependencies. The parent project's
# add_definitions() runs after add_subdirectory(base) and therefore does
# NOT reach base — setting them here ensures base sources see them.
if(WIN32)
  target_compile_definitions(base_compile_config INTERFACE
    UNICODE
    _UNICODE
    NOMINMAX
    _CRT_SECURE_NO_WARNINGS
    _CRT_SECURE_NO_DEPRECATE
    _SCL_SECURE_NO_DEPRECATE
    _WINSOCKAPI_
    __STDC_CONSTANT_MACROS
    __STDC_FORMAT_MACROS
  )
endif()

# ---------------------------------------------------------------------------
# abseil-cpp (external, from //third_party/abseil-cpp)
# ---------------------------------------------------------------------------
# Allow the parent project to override the abseil-cpp source directory.
# Default: ${CHROMIUM_SRC}/third_party/abseil-cpp (base's in-tree location
# when consumed as part of a full Chromium checkout). When base is consumed
# as a subdirectory of another project (e.g. tin), the parent must set
# BASE_ABSL_SOURCE_DIR before calling add_subdirectory(...).
#
# Use CACHE so a parent-set value wins over the default here even across
# reconfigures; FORCE only on the default branch so we never overwrite an
# explicit parent setting.
if(NOT BASE_ABSL_SOURCE_DIR)
  set(BASE_ABSL_SOURCE_DIR ${CHROMIUM_SRC}/third_party/abseil-cpp
      CACHE PATH "Path to abseil-cpp source tree")
endif()
message(STATUS "base: BASE_ABSL_SOURCE_DIR = ${BASE_ABSL_SOURCE_DIR}")

# Base headers include abseil via the Chromium-style path
# `#include "third_party/abseil-cpp/absl/..."`. In a full Chromium checkout
# this resolves via ${CHROMIUM_SRC} (which contains both base/ and
# third_party/abseil-cpp/). When base is consumed as a subdirectory of
# another project (e.g. tin), abseil-cpp lives at ${BASE_ABSL_SOURCE_DIR}
# which is NOT under ${CHROMIUM_SRC}/third_party/. Adding
# ${BASE_ABSL_SOURCE_DIR}/../.. makes the "third_party/abseil-cpp/" prefix
# resolve to the actual abseil source tree.
target_include_directories(base_compile_config INTERFACE
  ${BASE_ABSL_SOURCE_DIR}/../..
)

set(ABSL_PROPAGATE_CXX_STD ON CACHE BOOL "" FORCE)
set(ABSL_BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(ABSL_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
# Hide the bazel-style copts file lookups behind the build dir.
if(EXISTS ${BASE_ABSL_SOURCE_DIR}/CMakeLists.txt)
  add_subdirectory(
    ${BASE_ABSL_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/abseil-cpp
    EXCLUDE_FROM_ALL
  )
else()
  message(FATAL_ERROR "abseil-cpp not found at ${BASE_ABSL_SOURCE_DIR}")
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
  # Per-platform sources we don't build (common to all platforms).
  # Note: _win.cc and /win/ are only excluded on non-Windows; _posix.cc
  # is only excluded on Windows. See the platform-specific block below.
  "/android/"
  "/apple/"
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
  # Message pump / message loop infrastructure — not needed in the minimal
  # core build. Cascade-excludes the parts of //base/task and //base/threading
  # that depend on MessagePump (sequence_manager, thread_pool,
  # single_thread_task_executor, current_thread, Thread, RunLoop, etc.).
  # The leaf task_runner / sequenced_task_runner / delayed_task_handle
  # classes stay — they don't reference MessagePump and many low-level
  # users (logging, file_util, timer, ...) only need their static helpers.
  "/message_loop/"
  "/task/sequence_manager/"
  "/task/thread_pool/"
  "task/single_thread_task_executor.cc"
  "task/current_thread.cc"
  "/threading/thread.cc"
  "run_loop.cc"
  "files/file_descriptor_watcher_posix.cc"
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
  # process/kill, launch, memory, process_iterator — excluded entirely
  # (cross-platform .cc + _posix.cc + _win.cc variants) because they pull
  # in subprocess/launch infrastructure not needed in the minimal build.
  # Using broad patterns (without .cc suffix) catches all platform variants.
  "process/kill"
  "process/launch"
  "process/memory"
  "process/process_iterator"
  # file_path_watcher requires blink (catch all platform variants)
  "file_path_watcher"
  # sync_socket requires blink (catch all platform variants)
  "sync_socket"
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
  # stack_trace_nacl.cc duplicates base::debug::StackTrace definitions
  # already provided by stack_trace.cc on non-NaCl platforms. In jumbo
  # mode, both .o files can be pulled in simultaneously → multiple
  # definition linker errors.
  "stack_trace_nacl.cc"
  # sys_info_nacl.cc duplicates SysInfo::VMAllocationGranularity() which is
  # already provided by sys_info_posix.cc on Linux.
  "sys_info_nacl.cc"
  # Fallback Semaphore implementation used only when neither Win/iOS/POSIX/Fuchsia
  # is selected. Linux uses semaphore_posix.cc.
  "semaphore_default.cc"
  # Test helpers that depend on gmock (not pulled in for the minimal build).
  "test_task_factory.cc"
  "test_utils.cc"
  # Compile-test files (e.g. dynamic_annotations_compiletest.cc) are
  # build-time checks, not library sources.
  "_compiletest.cc"
  # mock_timer.cc depends on base/test/test_simple_task_runner.h which lives
  # in the excluded /test/ directory.
  "mock_timer.cc"
  # ICU-backed time helpers — we don't link ICU in the minimal build.
  "time_exploded_icu.cc"
  # Required for partition_alloc only
  "allocator/miracle_parameter.cc"
  "allocator/partition_alloc_features.cc"
  "allocator/partition_alloc_support.cc"
  "allocator/early_zone_registration_apple.cc"
)

# ---------------------------------------------------------------------------
# Platform-specific source exclusions
# ---------------------------------------------------------------------------
# On non-Windows (Linux/macOS): exclude _win.cc, _winrt.cc, /win/.
# On Windows: exclude _posix.cc (POSIX API users that won't compile on
#   Windows) and _winrt.cc (UWP-only). Also exclude Windows-specific
#   _win.cc files whose cross-platform .cc counterpart is already excluded
#   above (they would have unresolved symbols without the base .cc file).
if(WIN32)
  list(APPEND _exclude_substrings
    # POSIX-specific sources — won't compile on Windows (use POSIX APIs
    # like unistd.h, dlfcn.h, pthread, etc. without guards).
    "_posix.cc"
    # UWP-only sources — not supported in our minimal desktop build.
    "_winrt.cc"
    # Linux-specific sources — on Windows, _win.cc variants are used.
    # Use broad pattern to catch _linux.cc, _linux_base.cc, etc.
    "_linux"
    # NaCl-specific sources that include <unistd.h> or other POSIX headers.
    # (Most *_nacl.cc files are self-contained C++ ports we want to keep,
    # but these specific ones use POSIX APIs.)
    "page_size_nacl.cc"
    # ELF/DWARF debug info readers — Linux/ELF only (include <elf.h>,
    # <unistd.h>, use ssize_t, google::ReadFromOffset, etc.).
    "buffered_dwarf_reader.cc"
    "dwarf_line_no.cc"
    "elf_reader.cc"
    "test_elf_image_builder"
    # allocation_trace requires a stack tracer backend (frame pointers or
    # Linux). On Windows we exclude the profiler, so this hits #error.
    "allocation_trace.cc"
    # file_descriptor_store uses base::ScopedFD (POSIX-only).
    "file_descriptor_store"
    # linux_util uses pid_t and other POSIX/Linux types.
    "linux_util"
    # command_line.cc has a static_assert that uses a non-constexpr
    # variable (kSwitchPrefixes); this fails on clang-cl. Excluding it
    # is safe for the minimal build.
    "command_line.cc"
    # WMI support requires COM initialization and has complex dependencies
    # not needed in the minimal build.
    "wmi.cc"
    # process_startup_helper depends on CommandLine (excluded above).
    "process_startup_helper.cc"
    # Unix/Linux-specific directories (arpa/inet.h, unistd.h, etc.).
    "/nix/"
    "/posix/"
    # nodebug_assertion.cc provides __libcpp_verbose_abort for libc++.
    # It includes <__config> (a libc++ internal header) not available with
    # the MSVC standard library used on Windows.
    "nodebug_assertion.cc"
    # os_compat_nacl.cc uses POSIX setenv/unsetenv not available on Windows.
    # (Most *_nacl.cc files are self-contained, but this one is not.)
    "os_compat_nacl.cc"
    # battery_level_provider_win.cc includes <initguid.h> which causes
    # GUID_DEVINTERFACE_* redefinition errors with winioctl.h.
    # Battery monitoring is not needed in the minimal build.
    "battery_level_provider_win.cc"
    # scoped_handle_test_dll.cc is a test DLL that requires the generated
    # base_win_buildflags.h header (not produced in our minimal CMake build).
    "scoped_handle_test_dll.cc"
    # win_util.cc includes <lm.h> and other Windows SDK headers that require
    # the full <windows.h> (not just windows_types.h). It also depends on
    # MDM registration, setup API, shell scaling, etc. — not needed in the
    # minimal build.
    "win/win_util.cc"
    # scoped_pdh_query.cc includes <lm.h> via scoped_pdh_query.h, which
    # requires the full <windows.h>. PDH (Performance Data Helper) is not
    # needed in the minimal build.
    "scoped_pdh_query.cc"
  )
else()
  list(APPEND _exclude_substrings
    "_win.cc"
    "_winrt.cc"
    "/win/"
  )
endif()

# Files whose basename matches the stub we explicitly keep (override the
# broad "/trace_event/" exclusion above).
set(_keep_files
  "${BASE_ROOT}/trace_event/trace_event_stub.cc"
  "${BASE_ROOT}/trace_event/trace_event_stub.h"
  "${BASE_ROOT}/files/file_path_watcher_stub.cc"
  # Stubs for partition_alloc symbols (e.g. TerminateBecauseOutOfMemory)
  # referenced by //base when partition_alloc proper is not built.
  "${BASE_ROOT}/cmake/partition_alloc_stubs.cc"
  # Stubs for task/threading symbols (CurrentUIThread::IsSet(),
  # ThreadPoolInstance::Get(), ThreadPoolImpl::*TaskRunner factories, etc.)
  # whose implementations live in the excluded /message_loop/,
  # /task/sequence_manager/, /task/thread_pool/, task/current_thread.cc,
  # /threading/thread.cc, run_loop.cc files.
  "${BASE_ROOT}/cmake/task_stubs.cc"
  # Stubs for process/memory symbols (UncheckedMalloc, UncheckedCalloc,
  # UncheckedFree, EnableTerminationOnOutOfMemory, etc.) whose real
  # implementations live in the excluded process/memory*.cc files.
  # Needed because metrics/persistent_histogram_storage.cc and other
  # sources call base::UncheckedMalloc().
  "${BASE_ROOT}/process/memory_stubs.cc"
  # Stub for PowerMonitorDeviceSource::IsOnBatteryPower() — the real
  # implementation lives in platform-specific _win.cc (excluded on Linux).
  # Without this stub, any jumbo unit containing code that references
  # PowerMonitor::IsOnBatteryPower() fails to link.
  "${BASE_ROOT}/power_monitor/power_monitor_device_source_stub.cc"
)
# Windows-specific stubs for symbols whose implementations live in excluded
# files (win/win_util.cc, command_line.cc, /task/thread_pool/, partition_alloc
# page_allocator). Only compiled on Windows — on Linux the actual POSIX
# implementations are compiled instead.
if(WIN32)
  list(APPEND _keep_files "${BASE_ROOT}/cmake/win_stubs.cc")
endif()

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
# Jumbo (unity) build
# ---------------------------------------------------------------------------
# When BASE_JUMBO_BUILD is ON, merge multiple .cc files into a small number
# of "jumbo" translation units to dramatically reduce compilation time.
# Each jumbo unit #includes up to BASE_JUMBO_FILE_LIMIT source files.
#
# Files that cannot safely share a translation unit are listed in
# _jumbo_excludes and kept as standalone compilation units.  These are
# *jumbo-only* issues — in normal (non-jumbo) mode the linker only pulls
# the .o files that are actually referenced, so unreferenced main()
# functions or undefined-symbol calls never cause errors.
option(BASE_JUMBO_BUILD "Enable jumbo (unity) build for faster compilation" OFF)
set(BASE_JUMBO_FILE_LIMIT 50 CACHE STRING "Max source files per jumbo unit")

if(BASE_JUMBO_BUILD)
  include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/jumbo_build.cmake)

  # Patterns matched as substrings against the full source path.
  set(_jumbo_excludes
    # --- Stub files: small, define specific symbols, no jumbo benefit. ---
    "cmake/win_stubs.cc"
    "cmake/task_stubs.cc"
    "cmake/partition_alloc_stubs.cc"
    "trace_event_stub.cc"
    "file_path_watcher_stub.cc"
    # --- Files that define main(): in jumbo mode the whole merged .o is
    #     pulled in by any referenced symbol, causing multiple-main link
    #     errors.  In normal mode these .o files are never referenced and
    #     silently dropped by the linker. ---
    "_example.cc"
    "_benchmark.cc"
    "perftest"
    "build_utf8_validator_tables.cc"
    "smoke_test_main.cc"
    # --- NaCl alternative implementations: define the same symbols as
    #     their _posix.cc / _win.cc counterparts.  In normal mode the
    #     linker picks one .o; in jumbo mode they share a TU →
    #     redefinition errors. ---
    "stack_trace_nacl.cc"
    "platform_thread_nacl.cc"
    "page_size_nacl.cc"
    "sync_socket_nacl.cc"
    "rand_util_nacl.cc"
    "sys_info_nacl.cc"
    "os_compat_nacl.cc"
    # --- enterprise_util.cc calls IsManagedDevice()/IsEnterpriseDevice()
    #     which have no Linux implementation.  In normal mode the .o is
    #     never referenced; in jumbo mode it's merged with referenced
    #     code → undefined-symbol link error. ---
    "enterprise_util.cc"
    # --- task/post_job.cc references JobTaskSource and
    #     PooledTaskRunnerDelegate (defined in excluded /task/thread_pool/).
    #     In normal mode the .o is never referenced; in jumbo mode it's
    #     merged with referenced code -> undefined-symbol link error. ---
    "task/post_job.cc"
    # --- task/single_thread_task_runner.cc references
    #     ScopedDisallowRunningRunLoop (defined in excluded run_loop.cc).
    #     In normal mode the .o is never referenced; in jumbo mode it's
    #     merged with referenced code -> undefined-symbol link error. ---
    "task/single_thread_task_runner.cc"
    # --- nodebug_assertion.cc includes <__config> (libc++ internal). ---
    "nodebug_assertion.cc"
  )

  enable_jumbo_build(
    SOURCES_VAR    BASE_CC
    BUILD_DIR      ${CMAKE_BINARY_DIR}/jumbo/base
    INCLUDE_ROOT   ${CHROMIUM_SRC}
    FILE_LIMIT     ${BASE_JUMBO_FILE_LIMIT}
    EXCLUDE_FILES  ${_jumbo_excludes}
  )
endif()

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
)

# Platform-specific link libraries.
if(UNIX)
  target_link_libraries(base PRIVATE rt)
endif()
if(WIN32)
  target_link_libraries(base PRIVATE
    psapi
    ws2_32
    dbghelp
    shlwapi
    ntdll
    user32
    kernel32
    advapi32
    version
    powrprof
    winmm      # timeBeginPeriod/timeEndPeriod/timeGetTime (time_win.cc)
    delayimp   # __HrLoadAllImportsForDll (pe_image.cc)
  )
endif()

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

# GCC/Clang-style warning suppressions. clang-cl on Windows also accepts
# these flags, but pure MSVC does not. Guard with compiler id check.
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
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
endif()

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
