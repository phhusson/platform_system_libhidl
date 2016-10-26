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
#include <cutils/properties.h>
#include <hwbinder/Parcel.h>
#include <tuple>
#include <utils/Errors.h>
#include <utils/RefBase.h>
#include <utils/StrongPointer.h>

namespace android {
namespace hardware {

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

    status_t readEmbeddedFromParcel(
            const Parcel &parcel, size_t parentHandle, size_t parentOffset);

    status_t writeEmbeddedToParcel(
            Parcel *parcel, size_t parentHandle, size_t parentOffset) const;

    // offsetof(hidl_string, mBuffer) exposed since mBuffer is private.
    static const size_t kOffsetOfBuffer;

private:
    const char *mBuffer;
    size_t mSize;  // NOT including the terminating '\0'.
    bool mOwnsBuffer; // if true then mBuffer is a mutable char *

    // copy from data with size. Assume that my memory is freed
    // (through clear(), for example)
    void copyFrom(const char *data, size_t size);
    // move from another hidl_string
    void moveFrom(hidl_string &&);
};

////////////////////////////////////////////////////////////////////////////////

template<typename T>
struct hidl_vec {
    hidl_vec()
        : mBuffer(NULL),
          mSize(0),
          mOwnsBuffer(true) {
    }

    hidl_vec(const hidl_vec<T> &other) : hidl_vec() {
        *this = other;
    }

    hidl_vec(hidl_vec<T> &&other) {
        *this = static_cast<hidl_vec &&>(other);
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
        mSize = size;
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
        T *newBuffer = new T[size];

        for (size_t i = 0; i < std::min(size, mSize); ++i) {
            newBuffer[i] = mBuffer[i];
        }

        if (mOwnsBuffer) {
            delete[] mBuffer;
        }
        mBuffer = newBuffer;

        mSize = size;
        mOwnsBuffer = true;
    }

    status_t readEmbeddedFromParcel(
            const Parcel &parcel,
            size_t parentHandle,
            size_t parentOffset,
            size_t *handle);

    status_t writeEmbeddedToParcel(
            Parcel *parcel,
            size_t parentHandle,
            size_t parentOffset,
            size_t *handle) const;

    status_t findInParcel(const Parcel &parcel, size_t *handle) const {
        return parcel.quickFindBuffer(mBuffer, handle);
    }


private:
    T *mBuffer;
    size_t mSize;
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

////////////////////////////////////////////////////////////////////////////////

template<typename T>
status_t hidl_vec<T>::readEmbeddedFromParcel(
        const Parcel &parcel,
        size_t parentHandle,
        size_t parentOffset,
        size_t *handle) {
    const void *ptr = parcel.readEmbeddedBuffer(
            handle,
            parentHandle,
            parentOffset + offsetof(hidl_vec<T>, mBuffer));

    return ptr != NULL ? OK : UNKNOWN_ERROR;
}

template<typename T>
status_t hidl_vec<T>::writeEmbeddedToParcel(
        Parcel *parcel,
        size_t parentHandle,
        size_t parentOffset,
        size_t *handle) const {
    return parcel->writeEmbeddedBuffer(
            mBuffer,
            sizeof(T) * mSize,
            handle,
            parentHandle,
            parentOffset + offsetof(hidl_vec<T>, mBuffer));
}

///////////////////////////// pointers for HIDL

template <typename T>
static status_t readEmbeddedReferenceFromParcel(
        T const* * /* bufptr */,
        const Parcel & parcel,
        size_t parentHandle,
        size_t parentOffset,
        size_t *handle,
        bool *shouldResolveRefInBuffer
    ) {
    // *bufptr is ignored because, if I am embedded in some
    // other buffer, the kernel should have fixed me up already.
    bool isPreviouslyWritten;
    status_t result = parcel.readEmbeddedReference(
        nullptr, // ignored, not written to bufptr.
        handle,
        parentHandle,
        parentOffset,
        &isPreviouslyWritten);
    // tell caller to run T::readEmbeddedToParcel and
    // T::readEmbeddedReferenceToParcel if necessary.
    // It is not called here because we don't know if these two are valid methods.
    *shouldResolveRefInBuffer = !isPreviouslyWritten;
    return result;
}

template <typename T>
static status_t writeEmbeddedReferenceToParcel(
        T const* buf,
        Parcel *parcel, size_t parentHandle, size_t parentOffset,
        size_t *handle,
        bool *shouldResolveRefInBuffer
        ) {

    if(buf == nullptr) {
        *shouldResolveRefInBuffer = false;
        return parcel->writeEmbeddedNullReference(handle, parentHandle, parentOffset);
    }

    // find whether the buffer exists
    size_t childHandle, childOffset;
    status_t result;
    bool found;

    result = parcel->findBuffer(buf, sizeof(T), &found, &childHandle, &childOffset);

    // tell caller to run T::writeEmbeddedToParcel and
    // T::writeEmbeddedReferenceToParcel if necessary.
    // It is not called here because we don't know if these two are valid methods.
    *shouldResolveRefInBuffer = !found;

    if(result != OK) {
        return result; // bad pointers and length given
    }
    if(!found) { // did not find it.
        return parcel->writeEmbeddedBuffer(buf, sizeof(T), handle,
                parentHandle, parentOffset);
    }
    // found the buffer. easy case.
    return parcel->writeEmbeddedReference(
            handle,
            childHandle,
            childOffset,
            parentHandle,
            parentOffset);
}

template <typename T>
static status_t readReferenceFromParcel(
        T const* *bufptr,
        const Parcel & parcel,
        size_t *handle,
        bool *shouldResolveRefInBuffer
    ) {
    bool isPreviouslyWritten;
    status_t result = parcel.readReference(reinterpret_cast<void const* *>(bufptr),
            handle, &isPreviouslyWritten);
    // tell caller to run T::readEmbeddedToParcel and
    // T::readEmbeddedReferenceToParcel if necessary.
    // It is not called here because we don't know if these two are valid methods.
    *shouldResolveRefInBuffer = !isPreviouslyWritten;
    return result;
}

template <typename T>
static status_t writeReferenceToParcel(
        T const *buf,
        Parcel * parcel,
        size_t *handle,
        bool *shouldResolveRefInBuffer
    ) {

    if(buf == nullptr) {
        *shouldResolveRefInBuffer = false;
        return parcel->writeNullReference(handle);
    }

    // find whether the buffer exists
    size_t childHandle, childOffset;
    status_t result;
    bool found;

    result = parcel->findBuffer(buf, sizeof(T), &found, &childHandle, &childOffset);

    // tell caller to run T::writeEmbeddedToParcel and
    // T::writeEmbeddedReferenceToParcel if necessary.
    // It is not called here because we don't know if these two are valid methods.
    *shouldResolveRefInBuffer = !found;

    if(result != OK) {
        return result; // bad pointers and length given
    }
    if(!found) { // did not find it.
        return parcel->writeBuffer(buf, sizeof(T), handle);
    }
    // found the buffer. easy case.
    return parcel->writeReference(handle,
        childHandle, childOffset);
}

// ----------------------------------------------------------------------
// Version functions
struct hidl_version {
public:
    constexpr hidl_version(uint16_t major, uint16_t minor) : mMajor(major), mMinor(minor) {}

    bool operator==(const hidl_version& other) {
        return (mMajor == other.get_major() && mMinor == other.get_minor());
    }

    constexpr uint16_t get_major() const { return mMajor; }
    constexpr uint16_t get_minor() const { return mMinor; }

    android::status_t writeToParcel(android::hardware::Parcel& parcel) const {
        return parcel.writeUint32(static_cast<uint32_t>(mMajor) << 16 | mMinor);
    }

    static hidl_version* readFromParcel(const android::hardware::Parcel& parcel) {
        uint32_t version;
        android::status_t status = parcel.readUint32(&version);
        if (status != OK) {
            return nullptr;
        } else {
            return new hidl_version(version >> 16, version & 0xFFFF);
        }
    }

private:
    uint16_t mMajor;
    uint16_t mMinor;
};

inline android::hardware::hidl_version make_hidl_version(uint16_t major, uint16_t minor) {
    return hidl_version(major,minor);
}

#if defined(__LP64__)
#define HAL_LIBRARY_PATH_SYSTEM "/system/lib64/hw/"
#define HAL_LIBRARY_PATH_VENDOR "/vendor/lib64/hw/"
#define HAL_LIBRARY_PATH_ODM "/odm/lib64/hw/"
#else
#define HAL_LIBRARY_PATH_SYSTEM "/system/lib/hw/"
#define HAL_LIBRARY_PATH_VENDOR "/vendor/lib/hw/"
#define HAL_LIBRARY_PATH_ODM "/odm/lib/hw/"
#endif

#define DECLARE_REGISTER_AND_GET_SERVICE(INTERFACE)                                      \
    static ::android::sp<I##INTERFACE> getService(                                       \
            const std::string &serviceName, bool getStub=false);                         \
    status_t registerAsService(                                                          \
            const std::string &serviceName);                                             \

#define IMPLEMENT_REGISTER_AND_GET_SERVICE(INTERFACE, LIB)                               \
    ::android::sp<I##INTERFACE> I##INTERFACE::getService(                                \
            const std::string &serviceName, bool getStub)                                \
    {                                                                                    \
        sp<I##INTERFACE> iface;                                                          \
        const struct timespec DELAY {1,0};                                               \
        unsigned retries = 3;                                                            \
        const sp<IServiceManager> sm = defaultServiceManager();                          \
        if (sm != nullptr && !getStub) {                                                 \
            do {                                                                         \
                sp<IBinder> binderIface =                                                \
                        sm->checkService(String16(serviceName.c_str()),                  \
                                         I##INTERFACE::version);                         \
                iface = IHw##INTERFACE::asInterface(binderIface);                        \
                if (iface != nullptr) {                                                  \
                    return iface;                                                        \
                }                                                                        \
                TEMP_FAILURE_RETRY(nanosleep(&DELAY, nullptr));                          \
            } while (retries--);                                                         \
        }                                                                                \
        int dlMode = RTLD_LAZY;                                                          \
        void *handle = dlopen(HAL_LIBRARY_PATH_ODM LIB, dlMode);                         \
        if (handle == nullptr) {                                                         \
            handle = dlopen(HAL_LIBRARY_PATH_VENDOR LIB, dlMode);                        \
        }                                                                                \
        if (handle == nullptr) {                                                         \
            handle = dlopen(HAL_LIBRARY_PATH_SYSTEM LIB, dlMode);                        \
        }                                                                                \
        if (handle == nullptr) {                                                         \
            return iface;                                                                \
        }                                                                                \
        I##INTERFACE* (*generator)(const char* name);                                    \
        *(void **)(&generator) = dlsym(handle, "HIDL_FETCH_I"#INTERFACE);                \
        if (generator) {                                                                 \
            iface = (*generator)(serviceName.c_str());                                   \
            if (iface != nullptr) {                                                      \
                iface = new Bs##INTERFACE(iface);                                        \
            }                                                                            \
        }                                                                                \
        return iface;                                                                    \
    }                                                                                    \
    status_t I##INTERFACE::registerAsService(                                            \
            const std::string &serviceName)                                              \
    {                                                                                    \
        sp<Bn##INTERFACE> binderIface = new Bn##INTERFACE(this);                         \
        const sp<IServiceManager> sm = defaultServiceManager();                          \
        return sm->addService(String16(serviceName.c_str()), binderIface,                \
                              I##INTERFACE::version);                                    \
    }

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

