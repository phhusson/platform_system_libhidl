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

#include <hidl/TaskRunner.h>
#include <thread>

namespace android {
namespace hardware {

TaskRunner::TaskRunner() {
    bool *running = mRunning = new bool();
    SynchronizedQueue<std::function<void(void)>> *q
            = mQueue = new SynchronizedQueue<std::function<void(void)>>();
    mThread = new std::thread([running, q] {
        *running = true;
        while (*running) {
            (q->wait_pop())();
        }
        delete q;
        delete running;
    });
}
TaskRunner::~TaskRunner() {
    bool *running = mRunning;
    std::thread *t = mThread;
    mThread->detach();
    mQueue->push([running, t] {
        *running = false;
        delete t;
    });
}

} // namespace hardware
} // namespace android

