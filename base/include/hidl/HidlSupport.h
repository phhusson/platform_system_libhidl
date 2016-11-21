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

#ifndef ANDROID_HIDL_SUPPORT_H
#define ANDROID_HIDL_SUPPORT_H

#include <algorithm>
#include <dirent.h>
#include <dlfcn.h>
#include <iterator>
#include <cutils/properties.h>
#include <functional>
#include <hidl/Status.h>
#include <map>
#include <tuple>
#include <utils/Errors.h>
#include <utils/RefBase.h>
#include <utils/StrongPointer.h>
#include <vector>

namespace android {
namespace hardware {

namespace details {

// hidl_log_base is a base class that templatized
// classes implemented in a header can inherit from,
// to avoid creating dependencies on liblog.
struct hidl_log_base {
    void logAlwaysFatal(const char *message);
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
} // namespace details

struct hidl_string {
    hidl_string();
    ~hidl_string();

    // copy constructor.
    hidl_string(const hidl_string &);
    // copy from a C-style string.
    hidl_string(const char *);
    // copy from an std::string.
    hidl_string(const std::string &);

    // move constructor.
    hidl_string(hidl_string &&);

    const char *c_str() const;
    size_t size() const;
    bool empty() const;

    // copy assignment operator.
    hidl_string &operator=(const hidl_string &);
    // copy from a C-style string.
    hidl_string &operator=(const char *s);
    // copy from an std::string.
    hidl_string &operator=(const std::string &);
    // move assignment operator.
    hidl_string &operator=(hidl_string &&other);
    // cast to std::string.
    operator std::string() const;
    // cast to C-style string. Caller is responsible
    // to maintain this hidl_string alive.
    operator const char *() const;

    void clear();

    // Reference an external char array. Ownership is _not_ transferred.
    // Caller is responsible for ensuring that underlying memory is valid
    // for the lifetime of this hidl_string.
    void setToExternal(const char *data, size_t size);

    // offsetof(hidl_string, mBuffer) exposed since mBuffer is private.
    static const size_t kOffsetOfBuffer;

private:
    details::hidl_pointer<const char> mBuffer;
    uint32_t mSize;  // NOT including the terminating '\0'.
    bool mOwnsBuffer; // if true then mBuffer is a mutable char *

    // copy from data with size. Assume that my memory is freed
    // (through clear(), for example)
    void copyFrom(const char *data, size_t size);
    // move from another hidl_string
    void moveFrom(hidl_string &&);
};

inline bool operator==(const hidl_string &hs1, const hidl_string &hs2) {
    return strcmp(hs1.c_str(), hs2.c_str()) == 0;
}

inline bool operator!=(const hidl_string &hs1, const hidl_string &hs2) {
    return !(hs1 == hs2);
}

inline bool operator==(const hidl_string &hs, const char *s) {
    return strcmp(hs.c_str(), s) == 0;
}

inline bool operator!=(const hidl_string &hs, const char *s) {
    return !(hs == s);
}

inline bool operator==(const char *s, const hidl_string &hs) {
    return strcmp(hs.c_str(), s) == 0;
}

inline bool operator!=(const char *s, const hidl_string &hs) {
    return !(s == hs);
}

////////////////////////////////////////////////////////////////////////////////

template<typename T>
struct hidl_vec : private details::hidl_log_base {
    hidl_vec()
        : mBuffer(NULL),
          mSize(0),
          mOwnsBuffer(true) {
    }

    hidl_vec(const hidl_vec<T> &other) : hidl_vec() {
        *this = other;
    }

    hidl_vec(hidl_vec<T> &&other)
    : mOwnsBuffer(false) {
        *this = std::move(other);
    }

    hidl_vec(const std::initializer_list<T> list)
            : mSize(list.size()),
              mOwnsBuffer(true) {
        mBuffer = new T[mSize];

        size_t idx = 0;
        for (auto it = list.begin(); it != list.end(); ++it) {
            mBuffer[idx++] = *it;
        }
    }

    hidl_vec(const std::vector<T> &other) : hidl_vec() {
        *this = other;
    }

    ~hidl_vec() {
        if (mOwnsBuffer) {
            delete[] mBuffer;
        }
        mBuffer = NULL;
    }

    // Reference an existing array, optionally taking ownership. It is the
    // caller's responsibility to ensure that the underlying memory stays
    // valid for the lifetime of this hidl_vec.
    void setToExternal(T *data, size_t size, bool shouldOwn = false) {
        if (mOwnsBuffer) {
            delete [] mBuffer;
        }
        mBuffer = data;
        if (size > UINT32_MAX) {
            logAlwaysFatal("external vector size exceeds 2^32 elements.");
        }
        mSize = static_cast<uint32_t>(size);
        mOwnsBuffer = shouldOwn;
    }

    T *data() {
        return mBuffer;
    }

    const T *data() const {
        return mBuffer;
    }

    T *releaseData() {
        if (!mOwnsBuffer && mSize > 0) {
            resize(mSize);
        }
        mOwnsBuffer = false;
        return mBuffer;
    }

    hidl_vec &operator=(hidl_vec &&other) {
        if (mOwnsBuffer) {
            delete[] mBuffer;
        }
        mBuffer = other.mBuffer;
        mSize = other.mSize;
        mOwnsBuffer = other.mOwnsBuffer;
        other.mOwnsBuffer = false;
        return *this;
    }

    hidl_vec &operator=(const hidl_vec &other) {
        if (this != &other) {
            if (mOwnsBuffer) {
                delete[] mBuffer;
            }
            copyFrom(other, other.mSize);
        }

        return *this;
    }

    // copy from an std::vector.
    hidl_vec &operator=(const std::vector<T> &other) {
        if (mOwnsBuffer) {
            delete[] mBuffer;
        }
        copyFrom(other, other.size());
        return *this;
    }

    // cast to an std::vector.
    operator std::vector<T>() const {
        std::vector<T> v(mSize);
        for (size_t i = 0; i < mSize; ++i) {
            v[i] = mBuffer[i];
        }
        return v;
    }

    size_t size() const {
        return mSize;
    }

    T &operator[](size_t index) {
        return mBuffer[index];
    }

    const T &operator[](size_t index) const {
        return mBuffer[index];
    }

    void resize(size_t size) {
        if (size > UINT32_MAX) {
            logAlwaysFatal("hidl_vec can't hold more than 2^32 elements.");
        }
        T *newBuffer = new T[size];

        for (size_t i = 0; i < std::min(static_cast<uint32_t>(size), mSize); ++i) {
            newBuffer[i] = mBuffer[i];
        }

        if (mOwnsBuffer) {
            delete[] mBuffer;
        }
        mBuffer = newBuffer;

        mSize = static_cast<uint32_t>(size);
        mOwnsBuffer = true;
    }

    // offsetof(hidl_string, mBuffer) exposed since mBuffer is private.
    static const size_t kOffsetOfBuffer;

    // Define std interator interface for walking the array contents
    // TODO:  it might be nice to implement a full featured random access iterator...
    class iterator : public std::iterator<std::bidirectional_iterator_tag, T>
    {
    public:
        iterator(T* ptr) : mPtr(ptr) { }
        iterator operator++()    { iterator i = *this; mPtr++; return i; }
        iterator operator++(int) { mPtr++; return *this; }
        iterator operator--()    { iterator i = *this; mPtr--; return i; }
        iterator operator--(int) { mPtr--; return *this; }
        T& operator*()  { return *mPtr; }
        T* operator->() { return mPtr; }
        bool operator==(const iterator& rhs) const { return mPtr == rhs.mPtr; }
        bool operator!=(const iterator& rhs) const { return mPtr != rhs.mPtr; }
    private:
        T* mPtr;
    };
    iterator begin() { return data(); }
    iterator end() { return data()+mSize; }

private:
    details::hidl_pointer<T> mBuffer;
    uint32_t mSize;
    bool mOwnsBuffer;

    // copy from an array-like object, assuming my resources are freed.
    template <typename Array>
    void copyFrom(const Array &data, size_t size) {
        mSize = size;
        mOwnsBuffer = true;
        if (mSize > 0) {
            mBuffer = new T[size];
            for (size_t i = 0; i < size; ++i) {
                mBuffer[i] = data[i];
            }
        } else {
            mBuffer = NULL;
        }
    }
};

template <typename T>
const size_t hidl_vec<T>::kOffsetOfBuffer = offsetof(hidl_vec<T>, mBuffer);

////////////////////////////////////////////////////////////////////////////////

namespace details {

    template<size_t SIZE1, size_t... SIZES>
    struct product {
        static constexpr size_t value = SIZE1 * product<SIZES...>::value;
    };

    template<size_t SIZE1>
    struct product<SIZE1> {
        static constexpr size_t value = SIZE1;
    };

    template<typename T, size_t SIZE1, size_t... SIZES>
    struct accessor {
        explicit accessor(T *base)
            : mBase(base) {
        }

        accessor<T, SIZES...> operator[](size_t index) {
            return accessor<T, SIZES...>(
                    &mBase[index * product<SIZES...>::value]);
        }

    private:
        T *mBase;
    };

    template<typename T, size_t SIZE1>
    struct accessor<T, SIZE1> {
        explicit accessor(T *base)
            : mBase(base) {
        }

        T &operator[](size_t index) {
            return mBase[index];
        }

    private:
        T *mBase;
    };

    template<typename T, size_t SIZE1, size_t... SIZES>
    struct const_accessor {
        explicit const_accessor(const T *base)
            : mBase(base) {
        }

        const_accessor<T, SIZES...> operator[](size_t index) {
            return const_accessor<T, SIZES...>(
                    &mBase[index * product<SIZES...>::value]);
        }

    private:
        const T *mBase;
    };

    template<typename T, size_t SIZE1>
    struct const_accessor<T, SIZE1> {
        explicit const_accessor(const T *base)
            : mBase(base) {
        }

        const T &operator[](size_t index) const {
            return mBase[index];
        }

    private:
        const T *mBase;
    };

}  // namespace details

////////////////////////////////////////////////////////////////////////////////

template<typename T, size_t SIZE1, size_t... SIZES>
struct hidl_array {
    hidl_array() = default;

    T *data() { return mBuffer; }
    const T *data() const { return mBuffer; }

    details::accessor<T, SIZES...> operator[](size_t index) {
        return details::accessor<T, SIZES...>(
                &mBuffer[index * details::product<SIZES...>::value]);
    }

    details::const_accessor<T, SIZES...> operator[](size_t index) const {
        return details::const_accessor<T, SIZES...>(
                &mBuffer[index * details::product<SIZES...>::value]);
    }

    using size_tuple_type = std::tuple<decltype(SIZE1), decltype(SIZES)...>;

    static constexpr size_tuple_type size() {
        return std::make_tuple(SIZE1, SIZES...);
    }

private:
    T mBuffer[details::product<SIZE1, SIZES...>::value];
};

template<typename T, size_t SIZE1>
struct hidl_array<T, SIZE1> {
    hidl_array() = default;
    hidl_array(const T *source) {
        memcpy(mBuffer, source, SIZE1 * sizeof(T));
    }

    T *data() { return mBuffer; }
    const T *data() const { return mBuffer; }

    T &operator[](size_t index) {
        return mBuffer[index];
    }

    const T &operator[](size_t index) const {
        return mBuffer[index];
    }

    static constexpr size_t size() { return SIZE1; }

private:
    T mBuffer[SIZE1];
};

// ----------------------------------------------------------------------
// Version functions
struct hidl_version {
public:
    constexpr hidl_version(uint16_t major, uint16_t minor) : mMajor(major), mMinor(minor) {}

    bool operator==(const hidl_version& other) const {
        return (mMajor == other.get_major() && mMinor == other.get_minor());
    }

    constexpr uint16_t get_major() const { return mMajor; }
    constexpr uint16_t get_minor() const { return mMinor; }

private:
    uint16_t mMajor;
    uint16_t mMinor;
};

inline android::hardware::hidl_version make_hidl_version(uint16_t major, uint16_t minor) {
    return hidl_version(major,minor);
}

struct IBase : virtual public RefBase {
    virtual bool isRemote() const = 0;
    // HIDL reserved methods follow.
    virtual ::android::hardware::Return<void> interfaceChain(
            std::function<void(const hidl_vec<hidl_string>&)> _hidl_cb) = 0;
    // descriptor for HIDL reserved methods.
    static const char* descriptor;
};

#if defined(__LP64__)
#define HAL_LIBRARY_PATH_SYSTEM "/system/lib64/hw/"
#define HAL_LIBRARY_PATH_VENDOR "/vendor/lib64/hw/"
#define HAL_LIBRARY_PATH_ODM "/odm/lib64/hw/"
#else
#define HAL_LIBRARY_PATH_SYSTEM "/system/lib/hw/"
#define HAL_LIBRARY_PATH_VENDOR "/vendor/lib/hw/"
#define HAL_LIBRARY_PATH_ODM "/odm/lib/hw/"
#endif

#define DECLARE_SERVICE_MANAGER_INTERACTIONS(INTERFACE)                                  \
    static ::android::sp<I##INTERFACE> getService(                                       \
            const std::string &serviceName, bool getStub=false);                         \
    ::android::status_t registerAsService(const std::string &serviceName);               \
    static bool registerForNotifications(                                                \
        const std::string &serviceName,                                                  \
        const ::android::sp<::android::hidl::manager::V1_0::IServiceNotification>        \
                  &notification);                                                        \

// ----------------------------------------------------------------------
// Class that provides Hidl instrumentation utilities.
struct HidlInstrumentor {
    // Event that triggers the instrumentation. e.g. enter of an API call on
    // the server/client side, exit of an API call on the server/client side
    // etc.
    enum InstrumentationEvent {
        SERVER_API_ENTRY = 0,
        SERVER_API_EXIT,
        CLIENT_API_ENTRY,
        CLIENT_API_EXIT,
        SYNC_CALLBACK_ENTRY,
        SYNC_CALLBACK_EXIT,
        ASYNC_CALLBACK_ENTRY,
        ASYNC_CALLBACK_EXIT,
        PASSTHROUGH_ENTRY,
        PASSTHROUGH_EXIT,
    };

    // Signature of the instrumentation callback function.
    using InstrumentationCallback = std::function<void(
            const InstrumentationEvent event,
            const char *package,
            const char *version,
            const char *interface,
            const char *method,
            std::vector<void *> *args)>;

    explicit HidlInstrumentor(const std::string &prefix);
    virtual ~HidlInstrumentor();

 protected:
    // Function that lookup and dynamically loads the hidl instrumentation
    // libraries and registers the instrumentation callback functions.
    //
    // The instrumentation libraries should be stored under any of the following
    // directories: HAL_LIBRARY_PATH_SYSTEM, HAL_LIBRARY_PATH_VENDOR and
    // HAL_LIBRARY_PATH_ODM. The name of instrumentation libraries should
    // follow pattern: ^profilerPrefix(.*).profiler.so$
    //
    // Each instrumentation library is expected to implement the instrumentation
    // function called HIDL_INSTRUMENTATION_FUNCTION.
    //
    // A no-op for user build.
    void registerInstrumentationCallbacks(
            const std::string &profilerPrefix,
            std::vector<InstrumentationCallback> *instrumentationCallbacks);

    // Utility function to determine whether a give file is a instrumentation
    // library (i.e. the file name follow the expected pattern).
    bool isInstrumentationLib(
            const std::string &profilerPrefix,
            const dirent *file);
    // A list of registered instrumentation callbacks.
    std::vector<InstrumentationCallback> mInstrumentationCallbacks;
    // Flag whether to enable instrumentation.
    bool mEnableInstrumentation;
};

}  // namespace hardware
}  // namespace android


#endif  // ANDROID_HIDL_SUPPORT_H

