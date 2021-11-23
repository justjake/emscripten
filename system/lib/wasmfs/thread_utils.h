/*
 * Copyright 2021 The Emscripten Authors.  All rights reserved.
 * Emscripten is available under two separate licenses, the MIT license and the
 * University of Illinois/NCSA Open Source License.  Both these licenses can be
 * found in the LICENSE file.
 */

#pragma once

#include <assert.h>
#include <emscripten.h>
#include <emscripten/threading.h>
#include <pthread.h>

#include <functional>
#include <thread>
#include <utility>

namespace emscripten {

// Helper class for generic sync-to-async conversion. Creating an instance of
// this class will spin up a pthread. You can then call invoke() to run code
// on that pthread. The work done on the pthread receives a callback method
// which lets you indicate when it finished working. The call to invoke() is
// synchronous, while the work done on the other thread can be asynchronous,
// which allows bridging async JS APIs to sync C++ code.
//
// This can be useful if you are in a location where blocking is possible (like
// a thread, or when using PROXY_TO_PTHREAD), but you have code that is hard to
// refactor to be async, but that requires some async operation (like waiting
// for a JS event).
class SyncToAsync {

// Public API
//==============================================================================
public:

  // Pass around the callback as a pointer to a std::function. Using a pointer
  // means that it can be sent easily to JS, as a void* parameter to a C API,
  // etc., and also means we do not need to worry about the lifetime of the
  // std::function in user code.
  using Callback = std::function<void()>*;

  //
  // Run some work on thread. This is a synchronous (blocking) call. The thread
  // where the work actually runs can do async work for us - all it needs to do
  // is call the given callback function when it is done.
  //
  // Note that you need to call the callback even if you are not async, as the
  // code here does not know if you are async or not. For example,
  //
  //  instance.invoke([](emscripten::SyncToAsync::Callback resume) {
  //    std::cout << "Hello from sync C++ on the pthread\n";
  //    (*resume)();
  //  });
  //
  // In the async case, you would call resume() at some later time.
  //
  // It is safe to call this method from multiple threads, as it locks itself.
  // That is, you can create an instance of this and call it from multiple
  // threads freely.
  //
  void invoke(std::function<void(Callback)> newWork);

//==============================================================================
// End Public API

private:
  std::unique_ptr<std::thread> thread;
  std::mutex mutex;
  std::condition_variable condition;
  std::function<void(Callback)> work;
  bool readyToWork = false;
  bool finishedWork;
  bool quit = false;
  std::unique_ptr<std::function<void()>> resume;
  std::mutex invokeMutex;

  // The child will be asynchronous, and therefore we cannot rely on RAII to
  // unlock for us, we must do it manually.
  std::unique_lock<std::mutex> childLock;

  static void* threadMain(void* arg) {
    emscripten_async_call(threadIter, arg, 0);
    return 0;
  }

  static void threadIter(void* arg) {
    auto* parent = (SyncToAsync*)arg;
    if (parent->quit) {
      pthread_exit(0);
    }
    // Wait until we get something to do.
    parent->childLock.lock();
    parent->condition.wait(parent->childLock, [&]() {
      return parent->readyToWork;
    });
    auto work = parent->work;
    parent->readyToWork = false;
    // Allocate a resume function, and stash it on the parent.
    parent->resume = std::make_unique<std::function<void()>>([parent, arg]() {
      // We are called, so the work was finished. Notify the caller.
      parent->finishedWork = true;
      parent->childLock.unlock();
      parent->condition.notify_one();
      // Look for more work. Doing this asynchronously ensures that we continue
      // after the current call stack unwinds (avoiding constantly adding to the
      // stack, and also running any remaining code the caller had, like
      // destructors). TODO: add an option to do a synchronous call here in some
      // cases, which would avoid the time delay caused by a browser setTimeout.
      emscripten_async_call(threadIter, arg, 0);
    });
    // Run the work function the user gave us. Give it a pointer to the resume
    // function.
    work(parent->resume.get());
  }

public:
  SyncToAsync() : childLock(mutex) {
    // The child lock is associated with the mutex, which takes the lock as we
    // connect them, and so we must free it here so that the child can use it.
    // Only the child will lock/unlock it from now on.
    childLock.unlock();

    // Create the thread after the lock is ready.
    thread = std::make_unique<std::thread>(threadMain, this);
  }

  ~SyncToAsync() {
    // Wake up the child to tell it to quit.
    invoke([&](Callback func){
      quit = true;
      (*func)();
    });

    thread->join();
  }
};

void SyncToAsync::invoke(std::function<void(Callback)> newWork) {
  // Use the invokeMutex to prevent more than one invoke being in flight at a
  // time, so that this is usable from multiple threads safely.
  std::lock_guard<std::mutex> invokeLock(invokeMutex);

  // Send the work over.
  std::unique_lock<std::mutex> lock(mutex);
  work = newWork;
  finishedWork = false;
  readyToWork = true;

  // Notify the thread and wait for it to complete.
  condition.notify_one();
  condition.wait(lock, [&]() {
    return finishedWork;
  });
}

} // namespace emscripten
