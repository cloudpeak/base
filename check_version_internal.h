// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_CHECK_VERSION_INTERNAL_H_
#define BASE_CHECK_VERSION_INTERNAL_H_

#define VERSION_MAJOR 9999
#define NEXT_VERSION_MAJOR 10000

// Largest milestone that has shipped without a particular CHECK being
// made fatal. Used by check.cc to decide whether a NotFatalUntil
// milestone has been exceeded. Set to a large value so that all
// NotFatalUntil CHECKs are always fatal in this standalone build.
#define BASE_CHECK_VERSION_INTERNAL VERSION_MAJOR

#endif  // BASE_CHECK_VERSION_INTERNAL_H_
