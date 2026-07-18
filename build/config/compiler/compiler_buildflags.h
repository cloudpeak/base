// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BUILD_CONFIG_COMPILER_COMPILER_BUILDFLAGS_H_
#define BUILD_CONFIG_COMPILER_COMPILER_BUILDFLAGS_H_

#include "build/buildflag.h"

// Whether to support stack unwinding with frame pointers. Disabled for the
// minimal CMake build (we rely on symbolize / libunwind via DWARF).
#define BUILDFLAG_INTERNAL_CAN_UNWIND_WITH_FRAME_POINTERS() (false)

// Symbol level used by Chromium GN builds. 0 = no symbols, 1 = minimal,
// 2 = full DWARF. 1 is enough for our purposes here.
#define BUILDFLAG_INTERNAL_SYMBOL_LEVEL() (1)

#endif  // BUILD_CONFIG_COMPILER_COMPILER_BUILDFLAGS_H_
