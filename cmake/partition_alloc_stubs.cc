// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Minimal stub for partition_alloc::TerminateBecauseOutOfMemory().
//
// The full partition_alloc library (including //base/allocator/
// partition_allocator/src/partition_alloc/oom.cc) is excluded from the
// CMake-based minimal core build of //base. However, several base sources
// (e.g. base/metrics/field_trial.cc via base/process/memory.h) reference
// this symbol, so we provide a no-crash replacement here that simply
// aborts the process — matching the contract that this function never
// returns on actual OOM, while keeping the link self-contained.

#include <cstddef>
#include <cstdlib>

#include "partition_alloc/oom.h"

namespace partition_alloc {

void TerminateBecauseOutOfMemory(size_t size) {
  // The real implementation crashes the process with IMMEDIATE_CRASH so the
  // crash server classifies the failure as OOM. We don't need that
  // classification for the minimal CMake build, so a plain abort() suffices.
  std::abort();
}

}  // namespace partition_alloc
