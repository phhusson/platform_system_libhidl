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