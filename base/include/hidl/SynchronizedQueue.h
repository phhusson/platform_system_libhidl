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

#ifndef ANDROID_SYNCHRONIZED_QUEUE_H
#define ANDROID_SYNCHRONIZED_QUEUE_H

#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

namespace android {
namespace hardware {
/* Threadsafe queue.
 */
template <typename T>
struct SynchronizedQueue {

    /* Gets an item from the front of the queue.
     *
     * Blocks until the item is available.
     */
    T wait_pop();

    /* Puts an item onto the end of the queue.
     */
    bool push(const T& item);

    /* Gets the size of the array.
     */
    size_t size();

    /* Sets the limit to the queue. Will fail
     * the push operation if the limit is reached.
     */
    void setLimit(size_t limit);

private:
    std::condition_variable mCondition;
    std::mutex mMutex;
    std::queue<T> mQueue;
    size_t mQueueLimit = SIZE_MAX;
};

template <typename T>
T SynchronizedQueue<T>::wait_pop() {
    std::unique_lock<std::mutex> lock(mMutex);

    mCondition.wait(lock, [this]{
        return !this->mQueue.empty();
    });

    T item = mQueue.front();
    mQueue.pop();

    return item;
}

template <typename T>
bool SynchronizedQueue<T>::push(const T &item) {
    bool success;
    {
        std::unique_lock<std::mutex> lock(mMutex);
        if (mQueue.size() < mQueueLimit) {
            mQueue.push(item);
            success = true;
        } else {
            success = false;
        }
    }

    mCondition.notify_one();
    return success;
}

template <typename T>
size_t SynchronizedQueue<T>::size() {
    std::unique_lock<std::mutex> lock(mMutex);

    return mQueue.size();
}

template <typename T>
void SynchronizedQueue<T>::setLimit(size_t limit) {
    std::unique_lock<std::mutex> lock(mMutex);

    mQueueLimit = limit;
}

} // namespace hardware
} // namespace android

#endif
