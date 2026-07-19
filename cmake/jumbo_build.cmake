# Copyright 2024 The Chromium Authors
# CMake module for jumbo (unity) build support.
#
# Jumbo build accelerates compilation by merging multiple .cc source files
# into a small number of "jumbo" translation units. Each jumbo file is a
# generated .cc that #includes several original .cc files. This reduces the
# overhead of repeatedly parsing the same headers across hundreds of files.
#
# In Chromium's GN build system this is controlled by `use_jumbo_build` with
# a default merge limit of 50 files per jumbo unit. This module mirrors that
# behavior for the CMake-based minimal build.
#
# Usage:
#   include(cmake/jumbo_build.cmake)
#   enable_jumbo_build(
#     SOURCES_VAR    BASE_CC        # variable NAME (not value) holding the source list
#     BUILD_DIR      ${CMAKE_BINARY_DIR}/jumbo/base
#     INCLUDE_ROOT   ${CHROMIUM_SRC}  # root for computing #include paths
#     FILE_LIMIT     50             # max files per jumbo unit (optional, default 50)
#     EXCLUDE_FILES  file1.cc file2.cc  # files to keep standalone (optional)
#   )
#
# After the call, the variable named by SOURCES_VAR is replaced with the
# generated jumbo files followed by the excluded standalone files.
#
# Which files should be excluded?
#   Any .cc file that cannot safely share a translation unit with others.
# Common reasons:
#   - Static / anonymous-namespace symbols with names that collide with
#     another file's symbols (e.g. two files both defining `namespace {
#     const char kFoo[] = ...; }`).
#   - Files that #define / #undef macros that would leak into subsequent
#     #included files in the same jumbo unit.
#   - Files that rely on being the sole translation unit (e.g. files with
#     #pragma comment(lib, ...) on MSVC).

# ---------------------------------------------------------------------------
# enable_jumbo_build()
# ---------------------------------------------------------------------------
# Generates jumbo .cc files and replaces the source list variable.
#
# Parameters:
#   SOURCES_VAR    (required) Name of the CMake variable holding the source list.
#   BUILD_DIR      (required) Directory where jumbo .cc files are generated.
#   INCLUDE_ROOT   (required) Root path for computing #include relative paths.
#   FILE_LIMIT     (optional) Max files per jumbo unit. Default: 50.
#   EXCLUDE_FILES  (optional) List of source file paths (or substrings to
#                  match against the full path) to keep standalone.
# ---------------------------------------------------------------------------
function(enable_jumbo_build)
  cmake_parse_arguments(
    _jumbo
    ""                          # options
    "SOURCES_VAR;BUILD_DIR;INCLUDE_ROOT;FILE_LIMIT"  # one-value
    "EXCLUDE_FILES"             # multi-value
    ${ARGN}
  )

  if(NOT _jumbo_SOURCES_VAR)
    message(FATAL_ERROR "enable_jumbo_build: SOURCES_VAR is required")
  endif()
  if(NOT _jumbo_BUILD_DIR)
    message(FATAL_ERROR "enable_jumbo_build: BUILD_DIR is required")
  endif()
  if(NOT _jumbo_INCLUDE_ROOT)
    message(FATAL_ERROR "enable_jumbo_build: INCLUDE_ROOT is required")
  endif()

  if(NOT _jumbo_FILE_LIMIT)
    set(_jumbo_FILE_LIMIT 50)
  endif()

  # Read the source list from the variable named by SOURCES_VAR.
  set(_sources ${${_jumbo_SOURCES_VAR}})

  # Split sources into jumbo-able and excluded (standalone) files.
  set(_jumbo_sources)
  set(_standalone_sources)
  foreach(f ${_sources})
    set(_exclude FALSE)
    foreach(pattern ${_jumbo_EXCLUDE_FILES})
      if(f MATCHES "${pattern}")
        set(_exclude TRUE)
        break()
      endif()
    endforeach()
    if(_exclude)
      list(APPEND _standalone_sources ${f})
    else()
      list(APPEND _jumbo_sources ${f})
    endif()
  endforeach()

  # Create the output directory.
  file(MAKE_DIRECTORY ${_jumbo_BUILD_DIR})

  # Generate jumbo files.
  set(_jumbo_files)
  set(_unit_index 0)
  set(_count_in_unit 0)
  set(_unit_content "")

  foreach(f ${_jumbo_sources})
    if(_count_in_unit EQUAL 0)
      set(_unit_content
"// Auto-generated jumbo file — do not edit.
// This file merges ${_jumbo_FILE_LIMIT} source files into a single
// translation unit to reduce header re-parsing overhead.
// Jumbo unit ${_unit_index}

")
    endif()

    # Compute the include path relative to INCLUDE_ROOT so that
    # #include "base/logging.cc" resolves via the -I${INCLUDE_ROOT} flag.
    file(RELATIVE_PATH _rel_inc ${_jumbo_INCLUDE_ROOT} ${f})
    string(APPEND _unit_content "#include \"${_rel_inc}\"\n")

    math(EXPR _count_in_unit "${_count_in_unit} + 1")

    if(_count_in_unit GREATER_EQUAL ${_jumbo_FILE_LIMIT})
      set(_jumbo_file ${_jumbo_BUILD_DIR}/jumbo_${_unit_index}.cc)
      file(WRITE ${_jumbo_file} ${_unit_content})
      list(APPEND _jumbo_files ${_jumbo_file})
      math(EXPR _unit_index "${_unit_index} + 1")
      set(_count_in_unit 0)
      set(_unit_content "")
    endif()
  endforeach()

  # Write the last partial unit if any files remain.
  if(_count_in_unit GREATER 0)
    set(_jumbo_file ${_jumbo_BUILD_DIR}/jumbo_${_unit_index}.cc)
    file(WRITE ${_jumbo_file} ${_unit_content})
    list(APPEND _jumbo_files ${_jumbo_file})
  endif()

  # Report statistics.
  list(LENGTH _jumbo_sources _n_jumbo_src)
  list(LENGTH _jumbo_files _n_units)
  list(LENGTH _standalone_sources _n_standalone)
  message(STATUS
    "jumbo: ${_n_jumbo_src} sources -> ${_n_units} jumbo units "
    "(limit=${_jumbo_FILE_LIMIT}), ${_n_standalone} standalone")

  # Replace the source list with jumbo files + standalone files.
  set(${_jumbo_SOURCES_VAR} ${_jumbo_files} ${_standalone_sources} PARENT_SCOPE)
endfunction()
