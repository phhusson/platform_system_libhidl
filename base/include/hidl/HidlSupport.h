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
#include <array>
#include <dirent.h>
#include <dlfcn.h>
#include <iterator>
#include <cutils/native_handle.h>
#include <cutils/properties.h>
#include <functional>
#include <hidl/HidlInternal.h>
#include <hidl/Status.h>
#include <map>
#include <stddef.h>
#include <tuple>
#include <type_traits>
#include <utils/Errors.h>
#include <utils/RefBase.h>
#include <utils/StrongPointer.h>
#include <vector>

namespace android {

// this file is included by all hidl interface, so we must forward declare the
// IMemory and IBase types.
namespace hidl {
namespace memory {
namespace V1_0 {
    struct IMemory;
}; // namespace V1_0
}; // namespace manager
}; // namespace hidl

namespace hidl {
namespace base {
namespace V1_0 {
    struct IBase;
}; // namespace V1_0
}; // namespace base
}; // namespace hidl

namespace hardware {

// hidl_death_recipient is a callback interfaced that can be used with
// linkToDeath() / unlinkToDeath()
struct hidl_death_recipient : public virtual RefBase {
    virtual void serviceDied(uint64_t cookie,
            const ::android::wp<::android::hidl::base::V1_0::IBase>& who) = 0;
};

// hidl_handle wraps a pointer to a native_handle_t in a hidl_pointer,
// so that it can safely be transferred between 32-bit and 64-bit processes.
struct hidl_handle {
    hidl_handle() {
        mHandle = nullptr;
    }
    ~hidl_handle() {
    }

    // copy constructors.
    hidl_handle(const native_handle_t *handle) {
        mHandle = handle;
    }

    hidl_handle(const hidl_handle &other) {
        mHandle = other.mHandle;
    }

    // move constructor.
    hidl_handle(hidl_handle &&other) {
        *this = std::move(other);
    }

    // assingment operators
    hidl_handle &operator=(const hidl_handle &other) {
        mHandle = other.mHandle;
        return *this;
    }

    hidl_handle &operator=(const native_handle_t *native_handle) {
        mHandle = native_handle;
        return *this;
    }

    hidl_handle &operator=(hidl_handle &&other) {
        mHandle = other.mHandle;
        other.mHandle = nullptr;
        return *this;
    }

    const native_handle_t* operator->() const {
        return mHandle;
    }
    // implicit conversion to const native_handle_t*
    operator const native_handle_t *() const {
        return mHandle;
    }
    // explicit conversion
    const native_handle_t *getNativeHandle() const {
        return mHandle;
    }
private:
    details::hidl_pointer<const native_handle_t> mHandle;
};

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

// hidl_memory is a structure that can be used to transfer
// pieces of shared memory between processes. The assumption
// of this object is that the memory remains accessible as
// long as the file descriptors in the enclosed mHandle
// - as well as all of its cross-process dups() - remain opened.
struct hidl_memory {

    hidl_memory() : mOwnsHandle(false), mHandle(nullptr), mSize(0), mName("") {
    }

    /**
     * Creates a hidl_memory object and takes ownership of the handle.
     */
    hidl_memory(const hidl_string &name, const hidl_handle &handle, size_t size)
       : mOwnsHandle(true),
         mHandle(handle),
         mSize(size),
         mName(name)
    {}

    // copy constructor
    hidl_memory(const hidl_memory& other) {
        *this = other;
    }

    // copy assignment
    hidl_memory &operator=(const hidl_memory &other) {
        if (this != &other) {
            cleanup();

            if (other.mHandle == nullptr) {
                mHandle = nullptr;
                mOwnsHandle = false;
            } else {
                mOwnsHandle = true;
                mHandle = native_handle_clone(other.mHandle);
            }
            mSize = other.mSize;
            mName = other.mName;
        }

        return *this;
    }

    // TODO move constructor/move assignment

    ~hidl_memory() {
        cleanup();
    }

    const native_handle_t* handle() const {
        return mHandle;
    }

    const hidl_string &name() const {
        return mName;
    }

    size_t size() const {
        return mSize;
    }

    // offsetof(hidl_memory, mHandle) exposed since mHandle is private.
    static const size_t kOffsetOfHandle;
    // offsetof(hidl_memory, mName) exposed since mHandle is private.
    static const size_t kOffsetOfName;

private:
    bool mOwnsHandle;
    hidl_handle mHandle;
    size_t mSize;
    hidl_string mName;

    void cleanup() {
        // TODO(b/33812533): native_handle_delete
        if (mOwnsHandle && mHandle != nullptr) {
            native_handle_close(mHandle);
        }
    }
};

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
            : mOwnsBuffer(true) {
        if (list.size() > UINT32_MAX) {
            logAlwaysFatal("hidl_vec can't hold more than 2^32 elements.");
        }
        mSize = static_cast<uint32_t>(list.size());
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

    // equality check, assuming that T::operator== is defined.
    bool operator==(const hidl_vec &other) const {
        if (mSize != other.size()) {
            return false;
        }
        for (size_t i = 0; i < mSize; ++i) {
            if (!(mBuffer[i] == other.mBuffer[i])) {
                return false;
            }
        }
        return true;
    }

    // inequality check, assuming that T::operator== is defined.
    inline bool operator!=(const hidl_vec &other) const {
        return !((*this) == other);
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

private:
    // Define std interator interface for walking the array contents
    template<bool is_const>
    class iter : public std::iterator<
            std::random_access_iterator_tag, /* Category */
            T,
            ptrdiff_t, /* Distance */
            typename std::conditional<is_const, const T *, T *>::type /* Pointer */,
            typename std::conditional<is_const, const T &, T &>::type /* Reference */>
    {
        using traits = std::iterator_traits<iter>;
        using ptr_type = typename traits::pointer;
        using ref_type = typename traits::reference;
        using diff_type = typename traits::difference_type;
    public:
        iter(ptr_type ptr) : mPtr(ptr) { }
        inline iter &operator++()    { mPtr++; return *this; }
        inline iter  operator++(int) { iter i = *this; mPtr++; return i; }
        inline iter &operator--()    { mPtr--; return *this; }
        inline iter  operator--(int) { iter i = *this; mPtr--; return i; }
        inline friend iter operator+(diff_type n, const iter &it) { return it.mPtr + n; }
        inline iter  operator+(diff_type n) const { return mPtr + n; }
        inline iter  operator-(diff_type n) const { return mPtr - n; }
        inline diff_type operator-(const iter &other) const { return mPtr - other.mPtr; }
        inline iter &operator+=(diff_type n) { mPtr += n; return *this; }
        inline iter &operator-=(diff_type n) { mPtr -= n; return *this; }
        inline ref_type operator*() const  { return *mPtr; }
        inline ptr_type operator->() const { return mPtr; }
        inline bool operator==(const iter &rhs) const { return mPtr == rhs.mPtr; }
        inline bool operator!=(const iter &rhs) const { return mPtr != rhs.mPtr; }
        inline bool operator< (const iter &rhs) const { return mPtr <  rhs.mPtr; }
        inline bool operator> (const iter &rhs) const { return mPtr >  rhs.mPtr; }
        inline bool operator<=(const iter &rhs) const { return mPtr <= rhs.mPtr; }
        inline bool operator>=(const iter &rhs) const { return mPtr >= rhs.mPtr; }
        inline ref_type operator[](size_t n) const { return mPtr[n]; }
    private:
        ptr_type mPtr;
    };
public:
    using iterator       = iter<false /* is_const */>;
    using const_iterator = iter<true  /* is_const */>;

    iterator begin() { return data(); }
    iterator end() { return data()+mSize; }
    const_iterator begin() const { return data(); }
    const_iterator end() const { return data()+mSize; }

private:
    details::hidl_pointer<T> mBuffer;
    uint32_t mSize;
    bool mOwnsBuffer;

    // copy from an array-like object, assuming my resources are freed.
    template <typename Array>
    void copyFrom(const Array &data, size_t size) {
        mSize = static_cast<uint32_t>(size);
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
    struct std_array {
        using type = std::array<typename std_array<T, SIZES...>::type, SIZE1>;
    };

    template<typename T, size_t SIZE1>
    struct std_array<T, SIZE1> {
        using type = std::array<T, SIZE1>;
    };

    template<typename T, size_t SIZE1, size_t... SIZES>
    struct accessor {

        using std_array_type = typename std_array<T, SIZE1, SIZES...>::type;

        explicit accessor(T *base)
            : mBase(base) {
        }

        accessor<T, SIZES...> operator[](size_t index) {
            return accessor<T, SIZES...>(
                    &mBase[index * product<SIZES...>::value]);
        }

        accessor &operator=(const std_array_type &other) {
            for (size_t i = 0; i < SIZE1; ++i) {
                (*this)[i] = other[i];
            }
            return *this;
        }

    private:
        T *mBase;
    };

    template<typename T, size_t SIZE1>
    struct accessor<T, SIZE1> {

        using std_array_type = typename std_array<T, SIZE1>::type;

        explicit accessor(T *base)
            : mBase(base) {
        }

        T &operator[](size_t index) {
            return mBase[index];
        }

        accessor &operator=(const std_array_type &other) {
            for (size_t i = 0; i < SIZE1; ++i) {
                (*this)[i] = other[i];
            }
            return *this;
        }

    private:
        T *mBase;
    };

    template<typename T, size_t SIZE1, size_t... SIZES>
    struct const_accessor {

        using std_array_type = typename std_array<T, SIZE1, SIZES...>::type;

        explicit const_accessor(const T *base)
            : mBase(base) {
        }

        const_accessor<T, SIZES...> operator[](size_t index) {
            return const_accessor<T, SIZES...>(
                    &mBase[index * product<SIZES...>::value]);
        }

        operator std_array_type() {
            std_array_type array;
            for (size_t i = 0; i < SIZE1; ++i) {
                array[i] = (*this)[i];
            }
            return array;
        }

    private:
        const T *mBase;
    };

    template<typename T, size_t SIZE1>
    struct const_accessor<T, SIZE1> {

        using std_array_type = typename std_array<T, SIZE1>::type;

        explicit const_accessor(const T *base)
            : mBase(base) {
        }

        const T &operator[](size_t index) const {
            return mBase[index];
        }

        operator std_array_type() {
            std_array_type array;
            for (size_t i = 0; i < SIZE1; ++i) {
                array[i] = (*this)[i];
            }
            return array;
        }

    private:
        const T *mBase;
    };

}  // namespace details

////////////////////////////////////////////////////////////////////////////////

// A multidimensional array of T's. Assumes that T::operator=(const T &) is defined.
template<typename T, size_t SIZE1, size_t... SIZES>
struct hidl_array {

    using std_array_type = typename details::std_array<T, SIZE1, SIZES...>::type;

    hidl_array() = default;

    // Copies the data from source, using T::operator=(const T &).
    hidl_array(const T *source) {
        for (size_t i = 0; i < elementCount(); ++i) {
            mBuffer[i] = source[i];
        }
    }

    // Copies the data from the given std::array, using T::operator=(const T &).
    hidl_array(const std_array_type &array) {
        details::accessor<T, SIZE1, SIZES...> modifier(mBuffer);
        modifier = array;
    }

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

    // equality check, assuming that T::operator== is defined.
    bool operator==(const hidl_array &other) const {
        for (size_t i = 0; i < elementCount(); ++i) {
            if (!(mBuffer[i] == other.mBuffer[i])) {
                return false;
            }
        }
        return true;
    }

    inline bool operator!=(const hidl_array &other) const {
        return !((*this) == other);
    }

    using size_tuple_type = std::tuple<decltype(SIZE1), decltype(SIZES)...>;

    static constexpr size_tuple_type size() {
        return std::make_tuple(SIZE1, SIZES...);
    }

    static constexpr size_t elementCount() {
        return details::product<SIZE1, SIZES...>::value;
    }

    operator std_array_type() const {
        return details::const_accessor<T, SIZE1, SIZES...>(mBuffer);
    }

private:
    T mBuffer[elementCount()];
};

// An array of T's. Assumes that T::operator=(const T &) is defined.
template<typename T, size_t SIZE1>
struct hidl_array<T, SIZE1> {

    using std_array_type = typename details::std_array<T, SIZE1>::type;

    hidl_array() = default;

    // Copies the data from source, using T::operator=(const T &).
    hidl_array(const T *source) {
        for (size_t i = 0; i < elementCount(); ++i) {
            mBuffer[i] = source[i];
        }
    }

    // Copies the data from the given std::array, using T::operator=(const T &).
    hidl_array(const std_array_type &array) : hidl_array(array.data()) {}

    T *data() { return mBuffer; }
    const T *data() const { return mBuffer; }

    T &operator[](size_t index) {
        return mBuffer[index];
    }

    const T &operator[](size_t index) const {
        return mBuffer[index];
    }

    // equality check, assuming that T::operator== is defined.
    bool operator==(const hidl_array &other) const {
        for (size_t i = 0; i < elementCount(); ++i) {
            if (!(mBuffer[i] == other.mBuffer[i])) {
                return false;
            }
        }
        return true;
    }

    inline bool operator!=(const hidl_array &other) const {
        return !((*this) == other);
    }

    static constexpr size_t size() { return SIZE1; }
    static constexpr size_t elementCount() { return SIZE1; }

    // Copies the data to an std::array, using T::operator=(T).
    operator std_array_type() const {
        std_array_type array;
        for (size_t i = 0; i < SIZE1; ++i) {
            array[i] = mBuffer[i];
        }
        return array;
    }

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

    bool operator<(const hidl_version& other) const {
        return (mMajor < other.get_major() ||
                (mMajor == other.get_major() && mMinor < other.get_minor()));
    }

    bool operator>(const hidl_version& other) const {
        return other < *this;
    }

    bool operator<=(const hidl_version& other) const {
        return !(*this > other);
    }

    bool operator>=(const hidl_version& other) const {
        return !(*this < other);
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

#if defined(__LP64__)
#define HAL_LIBRARY_PATH_SYSTEM "/system/lib64/hw/"
#define HAL_LIBRARY_PATH_VENDOR "/vendor/lib64/hw/"
#define HAL_LIBRARY_PATH_ODM "/odm/lib64/hw/"
#else
#define HAL_LIBRARY_PATH_SYSTEM "/system/lib/hw/"
#define HAL_LIBRARY_PATH_VENDOR "/vendor/lib/hw/"
#define HAL_LIBRARY_PATH_ODM "/odm/lib/hw/"
#endif

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
