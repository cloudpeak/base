// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BUILD_BUILD_CONFIG_H_
#define BUILD_BUILD_CONFIG_H_

#include "build/buildflag.h"

// Platform detection macros.
// For this CMake-based build we target Linux on x86_64 with clang/gcc.

#if defined(__linux__)
#define OS_LINUX 1
#define OS_POSIX 1
#endif

#if defined(__APPLE__)
#define OS_MAC 1
#define OS_MACOS 1
#define OS_POSIX 1
#define OS_BSD 1
#endif

#if defined(_WIN32)
#define OS_WIN 1
#endif

// Compiler detection. COMPILER_GCC covers GCC and Clang (which defines
// __GNUC__ for compatibility); MSVC is the alternative. This mirrors
// Chromium's //build/build_config.h.
#if defined(_MSC_VER)
#define COMPILER_MSVC 1
#else
#define COMPILER_GCC 1
#endif

#if defined(__clang__)
#define COMPILER_CLANG 1
#endif

// CPU architecture detection.
#if defined(__x86_64__) || defined(_M_X64)
#define ARCH_CPU_X86_FAMILY 1
#define ARCH_CPU_X86_64 1
#define ARCH_CPU_64_BITS 1
#define ARCH_CPU_LITTLE_ENDIAN 1
#elif defined(__i386__) || defined(_M_IX86)
#define ARCH_CPU_X86_FAMILY 1
#define ARCH_CPU_X86 1
#define ARCH_CPU_32_BITS 1
#define ARCH_CPU_LITTLE_ENDIAN 1
#elif defined(__aarch64__)
#define ARCH_CPU_ARM64 1
#define ARCH_CPU_64_BITS 1
#define ARCH_CPU_LITTLE_ENDIAN 1
#elif defined(__arm__)
#define ARCH_CPU_ARM 1
#define ARCH_CPU_32_BITS 1
#define ARCH_CPU_LITTLE_ENDIAN 1
#endif

// WCHAR_T width.
#if defined(__WCHAR_MAX__) && (__WCHAR_MAX__ <= 0xffff)
#define WCSIZE 2
#define WCHAR_T_IS_16_BIT 1
#else
#define WCSIZE 4
#define WCHAR_T_IS_32_BIT 1
#endif

// IS_* flags consumed via BUILDFLAG() — these are the modern Chromium
// buildflag-style accessor for the OS_* / platform predicates.
#if defined(OS_LINUX)
#define BUILDFLAG_INTERNAL_IS_LINUX() (true)
#else
#define BUILDFLAG_INTERNAL_IS_LINUX() (false)
#endif

#if defined(OS_POSIX)
#define BUILDFLAG_INTERNAL_IS_POSIX() (true)
#else
#define BUILDFLAG_INTERNAL_IS_POSIX() (false)
#endif

#if defined(OS_WIN)
#define BUILDFLAG_INTERNAL_IS_WIN() (true)
#else
#define BUILDFLAG_INTERNAL_IS_WIN() (false)
#endif

#if defined(OS_MAC) || defined(OS_MACOS)
#define BUILDFLAG_INTERNAL_IS_APPLE() (true)
#define BUILDFLAG_INTERNAL_IS_MAC() (true)
#else
#define BUILDFLAG_INTERNAL_IS_APPLE() (false)
#define BUILDFLAG_INTERNAL_IS_MAC() (false)
#endif

#if defined(OS_MAC) && defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
#define BUILDFLAG_INTERNAL_IS_IOS() (true)
#else
#define BUILDFLAG_INTERNAL_IS_IOS() (false)
#endif

#define BUILDFLAG_INTERNAL_IS_IOS_APP_EXTENSION() (false)

#if defined(__ANDROID__)
#define BUILDFLAG_INTERNAL_IS_ANDROID() (true)
#else
#define BUILDFLAG_INTERNAL_IS_ANDROID() (false)
#endif

#if defined(__Fuchsia__)
#define BUILDFLAG_INTERNAL_IS_FUCHSIA() (true)
#else
#define BUILDFLAG_INTERNAL_IS_FUCHSIA() (false)
#endif

#define BUILDFLAG_INTERNAL_IS_CHROMEOS() (false)
#define BUILDFLAG_INTERNAL_IS_CHROMEOS_ASH() (false)
#define BUILDFLAG_INTERNAL_IS_CASTOS() (false)
#define BUILDFLAG_INTERNAL_IS_CAST_ANDROID() (false)
#define BUILDFLAG_INTERNAL_IS_ROBOLECTRIC() (false)
#define BUILDFLAG_INTERNAL_IS_NACL() (false)
#define BUILDFLAG_INTERNAL_IS_NACL_SAIGO() (false)
#define BUILDFLAG_INTERNAL_IS_ASMJS() (false)
#define BUILDFLAG_INTERNAL_IS_BSD() (false)
#define BUILDFLAG_INTERNAL_IS_FREEBSD() (false)
#define BUILDFLAG_INTERNAL_IS_OPENBSD() (false)
#define BUILDFLAG_INTERNAL_IS_NETBSD() (false)
#define BUILDFLAG_INTERNAL_IS_SOLARIS() (false)
#define BUILDFLAG_INTERNAL_IS_AIX() (false)
#define BUILDFLAG_INTERNAL_IS_OZONE() (false)
#define BUILDFLAG_INTERNAL_IS_OZONE_X11() (false)

// Toolchain.
#define BUILDFLAG_INTERNAL_IS_CLANG() (true)

// Library ABI.
#define BUILDFLAG_INTERNAL_IS_COMPONENT_BUILD() (false)

#endif  // BUILD_BUILD_CONFIG_H_
