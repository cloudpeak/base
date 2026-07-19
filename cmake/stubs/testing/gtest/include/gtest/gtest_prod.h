// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Stub for testing/gtest/include/gtest/gtest_prod.h.
//
// The real gtest_prod.h defines the FRIEND_TEST macro that allows test
// classes to access private members of production classes. In the minimal
// CMake build (no gtest dependency), FRIEND_TEST is a no-op — production
// code compiles fine because the macro expands to nothing inside a
// "friend" declaration.
//
// This stub is put on the include path via base_compile_config so that
// base/gtest_prod_util.h resolves without a full gtest checkout.

#ifndef GTEST_INCLUDE_GTEST_GTEST_PROD_H_
#define GTEST_INCLUDE_GTEST_GTEST_PROD_H_

#define FRIEND_TEST(test_case_name, test_name)

#endif  // GTEST_INCLUDE_GTEST_GTEST_PROD_H_
