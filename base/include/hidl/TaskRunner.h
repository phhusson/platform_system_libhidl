/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef ANDROID_HIDL_TASK_RUNNER_H
#define ANDROID_HIDL_TASK_RUNNER_H

#include "SynchronizedQueue.h"
#include <thread>

namespace android {
namespace hardware {
namespace details {

/*
 * A background infinite loop that runs the Tasks push()'ed.
 * Just a simple single-threaded thread pool.
 */
class TaskRunner {
public:

    /* Kicks off the loop immediately. */
    TaskRunner();

    /*
     * Detaches the background thread and return immediately.
     * Tasks in the queue will continue to be done sequentially, and _after_
     * all tasks are done, the background thread releases the resources
     * (the queue, the std::thread object, etc.)
     */
    ~TaskRunner();

    /*
     * Add a task. Return true if successful, false if
     * the queue's size exceeds limit.
     */
    inline bool push(const std::function<void(void)> &t) {
        return this->mQueue->push(t);
    }

    /*
     * Sets the queue limit. Fails the push operation once the limit is reached.
     */
    inline void setLimit(size_t limit) {
        this->mQueue->setLimit(limit);
    }
private:

    // resources managed by the background thread.
    bool *mRunning;
    SynchronizedQueue<std::function<void(void)>> *mQueue;
    std::thread *mThread;
};

} // namespace details
} // namespace hardware
} // namespace android

#endif // ANDROID_HIDL_TASK_RUNNER_H
