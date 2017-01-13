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

#ifndef ANDROID_HIDL_INTERNAL_H
#define ANDROID_HIDL_INTERNAL_H

#include <cstdint>
#include <utility>

namespace android {
namespace hardware {
namespace details {

// hidl_log_base is a base class that templatized
// classes implemented in a header can inherit from,
// to avoid creating dependencies on liblog.
struct hidl_log_base {
    void logAlwaysFatal(const char *message) const;
};

// HIDL client/server code should *NOT* use this class.
//
// hidl_pointer wraps a pointer without taking ownership,
// and stores it in a union with a uint64_t. This ensures
// that we always have enough space to store a pointer,
// regardless of whether we're running in a 32-bit or 64-bit
// process.
template<typename T>
struct hidl_pointer {
    hidl_pointer()
        : mPointer(nullptr) {
    }
    hidl_pointer(T* ptr)
        : mPointer(ptr) {
    }
    hidl_pointer(const hidl_pointer<T>& other) {
        mPointer = other.mPointer;
    }
    hidl_pointer(hidl_pointer<T>&& other) {
        *this = std::move(other);
    }

    hidl_pointer &operator=(const hidl_pointer<T>& other) {
        mPointer = other.mPointer;
        return *this;
    }
    hidl_pointer &operator=(hidl_pointer<T>&& other) {
        mPointer = other.mPointer;
        other.mPointer = nullptr;
        return *this;
    }
    hidl_pointer &operator=(T* ptr) {
        mPointer = ptr;
        return *this;
    }

    operator T*() const {
        return mPointer;
    }
    explicit operator void*() const { // requires explicit cast to avoid ambiguity
        return mPointer;
    }
    T& operator*() const {
        return *mPointer;
    }
    T* operator->() const {
        return mPointer;
    }
    T &operator[](size_t index) {
        return mPointer[index];
    }
    const T &operator[](size_t index) const {
        return mPointer[index];
    }

private:
    union {
        T* mPointer;
        uint64_t _pad;
    };
};

}  // namespace details
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HIDL_INTERNAL_H
