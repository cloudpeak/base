// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Stubs for Windows-specific //base symbols whose implementations live in
// files excluded from the minimal CMake build (win/win_util.cc,
// command_line.cc, /task/thread_pool/). These stubs provide the symbol
// definitions needed by the remaining //base sources that reference them
// for optional features (logging, file path comparison, etc.) without
// pulling in the full implementation.

#include <string>

#include "base/command_line.h"
#include "base/memory/scoped_refptr.h"
#include "base/strings/string_piece.h"
#include "base/task/single_thread_task_runner.h"
#include "base/task/single_thread_task_runner_thread_mode.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool/thread_pool_impl.h"
#include "base/win/win_util.h"

namespace base {

// --- CommandLine (impl lives in command_line.cc, excluded on Windows) ---------
//
// logging.cc uses CommandLine to read vlog switches. With the real
// implementation excluded, return a safe "no switches" state.
// We avoid calling the CommandLine constructor (which lives in the excluded
// command_line.cc) by returning nullptr. Callers in logging.cc check for
// nullptr before use.

// static
CommandLine* CommandLine::ForCurrentProcess() {
  return nullptr;
}

// static
bool CommandLine::InitializedForCurrentProcess() {
  return false;
}

bool CommandLine::HasSwitch(StringPiece switch_string) const {
  (void)switch_string;
  return false;
}

bool CommandLine::HasSwitch(const char switch_constant[]) const {
  (void)switch_constant;
  return false;
}

std::string CommandLine::GetSwitchValueASCII(StringPiece switch_string) const {
  (void)switch_string;
  return std::string();
}

namespace internal {

// --- ThreadPoolImpl::CreateCOMSTATaskRunner (impl in /task/thread_pool/) ------
//
// Windows-only COM STA task runner factory. The full implementation lives in
// the excluded /task/thread_pool/ directory. Return nullptr to indicate no
// task runner is available, matching the "ThreadPool not started" contract.

scoped_refptr<SingleThreadTaskRunner>
ThreadPoolImpl::CreateCOMSTATaskRunner(
    const TaskTraits& traits,
    SingleThreadTaskRunnerThreadMode thread_mode) {
  (void)traits;
  (void)thread_mode;
  return nullptr;
}

}  // namespace internal

namespace win {

// --- IsUser32AndGdi32Available (impl in win/win_util.cc, excluded) -----------
//
// logging.cc (DisplayDebugMessageInDialog) and file_path.cc
// (CompareIgnoreCase) reference this on Windows. Return true to match the
// normal desktop behavior where user32.dll and gdi32.dll are available.

bool IsUser32AndGdi32Available() {
  return true;
}

}  // namespace win

}  // namespace base

// --- partition_alloc::ReleaseReservation (impl in page_allocator.cc) ---------
//
// base/memory/platform_shared_memory_mapper_win.cc references this symbol.
// The full partition_alloc page_allocator is excluded from the minimal build.
// A no-op stub is safe: ReleaseReservation is only called on OOM paths to
// release reserved memory; doing nothing is correct when no reservation
// was ever made.

#include "partition_alloc/page_allocator.h"

namespace partition_alloc {

bool ReleaseReservation() {
  return false;
}

}  // namespace partition_alloc

// --- std::_Literal_zero_is_expected (MSVC STL internal) ----------------------
//
// MSVC's <compare> header declares `void std::_Literal_zero_is_expected();`
// inside a consteval constructor. The function is never defined in the STL
// because consteval evaluation should never reach the call. However,
// clang-cl can emit a runtime reference to this symbol in some cases
// (notably for base::ScopedBlockingCallInternal and WaitableEvent). Provide
// a no-op definition so the link succeeds; the function should never be
// called at runtime.

namespace std {

void _Literal_zero_is_expected() {
  // No-op: this should never be called at runtime. The consteval constructor
  // in <compare> only calls it when the argument is non-zero, which is a
  // compile-time error.
}

}  // namespace std
