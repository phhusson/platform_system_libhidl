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

#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

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
    void push(const T& item);

    /* Gets the size of the array.
     */
    size_t size();

private:
    std::condition_variable mCondition;
    std::mutex mMutex;
    std::queue<T> mQueue;
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
void SynchronizedQueue<T>::push(const T &item) {
    {
        std::unique_lock<std::mutex> lock(mMutex);
        mQueue.push(item);
    }

    mCondition.notify_one();
}

template <typename T>
size_t SynchronizedQueue<T>::size() {
    std::unique_lock<std::mutex> lock(mMutex);

    return mQueue.size();
}
