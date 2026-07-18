// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include <cstdint>
#include <string>

#include "base/trace_event/heap_profiler_allocation_context_tracker.h"
#include "base/trace_event/memory_allocator_dump_guid.h"
#include "base/trace_event/trace_event_stub.h"

namespace base {
namespace trace_event {

ConvertableToTraceFormat::~ConvertableToTraceFormat() = default;

void TracedValue::AppendAsTraceFormat(std::string* out) const {}

MemoryDumpProvider::~MemoryDumpProvider() = default;

// static
constexpr const char* const MemoryDumpManager::kTraceCategory;

// Stub implementations of MemoryAllocatorDumpGuid ctors / ToString(). The
// real ones live in memory_allocator_dump_guid.cc (excluded here because the
// whole /trace_event/ tree is dropped). Only the std::string ctor is actually
// referenced by the rest of //base in the minimal build, but we provide all
// out-of-line members for completeness.
MemoryAllocatorDumpGuid::MemoryAllocatorDumpGuid(uint64_t guid) : guid_(guid) {}

MemoryAllocatorDumpGuid::MemoryAllocatorDumpGuid() : guid_(0u) {}

MemoryAllocatorDumpGuid::MemoryAllocatorDumpGuid(const std::string& guid_str)
    : guid_(0u) {
  // The real implementation hashes |guid_str| with SHA-1. Tracing is stubbed
  // out in this build, so a constant value is sufficient.
  (void)guid_str;
}

std::string MemoryAllocatorDumpGuid::ToString() const {
  return std::string();
}

// Stub for AllocationContextTracker::SetCurrentThreadName. The real
// implementation (heap_profiler_allocation_context_tracker.cc) is excluded
// along with the rest of /trace_event/. Heap profiling is disabled, so the
// call is a no-op.
void AllocationContextTracker::SetCurrentThreadName(const char* name) {
  (void)name;
}

}  // namespace trace_event
}  // namespace base

namespace perfetto {

TracedDictionary TracedValue::WriteDictionary() && {
  return TracedDictionary();
}

TracedArray TracedValue::WriteArray() && {
  return TracedArray();
}

TracedArray TracedDictionary::AddArray(StaticString) {
  return TracedArray();
}

TracedArray TracedDictionary::AddArray(DynamicString) {
  return TracedArray();
}

TracedDictionary TracedDictionary::AddDictionary(StaticString) {
  return TracedDictionary();
}

TracedDictionary TracedDictionary::AddDictionary(DynamicString) {
  return TracedDictionary();
}

TracedArray TracedArray::AppendArray() {
  return TracedArray();
}

TracedDictionary TracedArray::AppendDictionary() {
  return TracedDictionary();
}

}  // namespace perfetto
