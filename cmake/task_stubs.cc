// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Stubs for //base/task and //base/threading symbols whose implementations
// live in directories excluded from the minimal CMake build
// (/message_loop/, /task/sequence_manager/, /task/thread_pool/,
// task/single_thread_task_executor.cc, task/current_thread.cc,
// /threading/thread.cc, run_loop.cc).
//
// Each stub returns a safe "nothing is running" / "no task is posted" value
// so that the remaining //base sources that only use these APIs for
// optional features (e.g. detecting whether the current thread has a
// MessagePump, or posting a best-effort task that can be dropped) link
// cleanly without pulling in the full task/threading infrastructure.

#include "base/location.h"
#include "base/functional/callback.h"
#include "base/memory/scoped_refptr.h"
#include "base/run_loop.h"
#include "base/task/current_thread.h"
#include "base/task/single_thread_task_runner.h"
#include "base/task/single_thread_task_runner_thread_mode.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/task_runner.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool/environment_config.h"
#include "base/task/thread_pool/thread_pool_impl.h"
#include "base/task/thread_pool/thread_pool_instance.h"
#include "base/task/updateable_sequenced_task_runner.h"
#include "base/time/time.h"

namespace base {

// --- CurrentThread extensions (impls live in task/current_thread.cc) ---------

bool CurrentUIThread::IsSet() {
  return false;
}

bool CurrentIOThread::IsSet() {
  return false;
}

// --- ThreadPoolInstance (impls live in /task/thread_pool/) -------------------

ThreadPoolInstance* ThreadPoolInstance::Get() {
  return nullptr;
}

ThreadPoolInstance::ScopedFizzleBlockShutdownTasks::
    ScopedFizzleBlockShutdownTasks() = default;
ThreadPoolInstance::ScopedFizzleBlockShutdownTasks::
    ~ScopedFizzleBlockShutdownTasks() = default;

// --- ScopedDisallowRunningRunLoop (impl lives in run_loop.cc) ----------------
// The real implementation (in the excluded run_loop.cc) maintains a delegate
// pointer to temporarily disallow RunLoop::Run(). The stub provides a no-op
// version so the class is usable but does not enforce the constraint.
// In DCHECK builds the class has a const bool member that must be explicitly
// initialized (otherwise the default constructor is implicitly deleted).

#if DCHECK_IS_ON()
ScopedDisallowRunningRunLoop::ScopedDisallowRunningRunLoop()
    : current_delegate_(nullptr), previous_run_allowance_(false) {}
#else
ScopedDisallowRunningRunLoop::ScopedDisallowRunningRunLoop() = default;
#endif
ScopedDisallowRunningRunLoop::~ScopedDisallowRunningRunLoop() = default;

namespace internal {

// --- environment_config (impl lives in /task/thread_pool/) -------------------

bool CanUseBackgroundThreadTypeForWorkerThread() {
  return false;
}

// --- ThreadPoolImpl (impls live in /task/thread_pool/) -----------------------
//
// These mirror the public ThreadPool static API surface used by
// task/thread_pool.cc. Returning false / nullptr matches the contract of a
// ThreadPool that has not been started: PostTask fails, and no TaskRunner can
// be created.

bool ThreadPoolImpl::PostDelayedTask(const Location& from_here,
                                     const TaskTraits& traits,
                                     OnceClosure task,
                                     TimeDelta delay) {
  (void)from_here;
  (void)traits;
  (void)task;
  (void)delay;
  return false;
}

scoped_refptr<TaskRunner> ThreadPoolImpl::CreateTaskRunner(
    const TaskTraits& traits) {
  (void)traits;
  return nullptr;
}

scoped_refptr<SequencedTaskRunner> ThreadPoolImpl::CreateSequencedTaskRunner(
    const TaskTraits& traits) {
  (void)traits;
  return nullptr;
}

scoped_refptr<UpdateableSequencedTaskRunner>
ThreadPoolImpl::CreateUpdateableSequencedTaskRunner(const TaskTraits& traits) {
  (void)traits;
  return nullptr;
}

scoped_refptr<SingleThreadTaskRunner>
ThreadPoolImpl::CreateSingleThreadTaskRunner(
    const TaskTraits& traits,
    SingleThreadTaskRunnerThreadMode thread_mode) {
  (void)traits;
  (void)thread_mode;
  return nullptr;
}

}  // namespace internal

}  // namespace base
