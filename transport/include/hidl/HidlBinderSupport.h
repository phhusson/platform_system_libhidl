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

#ifndef ANDROID_HIDL_BINDER_SUPPORT_H
#define ANDROID_HIDL_BINDER_SUPPORT_H

#include <hidl/HidlSupport.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Static.h>
#include <hwbinder/IBinder.h>
#include <hwbinder/Parcel.h>

// Defines functions for hidl_string, hidl_version, Status, hidl_vec, MQDescriptor,
// etc. to interact with Parcel.

namespace android {
namespace hardware {

// ---------------------- hidl_memory

status_t readEmbeddedFromParcel(hidl_memory *memory,
        const Parcel &parcel, size_t parentHandle, size_t parentOffset);

status_t writeEmbeddedToParcel(const hidl_memory &memory,
        Parcel *parcel, size_t parentHandle, size_t parentOffset);

// ---------------------- hidl_string

status_t readEmbeddedFromParcel(hidl_string *string,
        const Parcel &parcel, size_t parentHandle, size_t parentOffset);

status_t writeEmbeddedToParcel(const hidl_string &string,
        Parcel *parcel, size_t parentHandle, size_t parentOffset);

// ---------------------- hidl_version

status_t writeToParcel(const hidl_version &version, android::hardware::Parcel& parcel);

// Caller is responsible for freeing the returned object.
hidl_version* readFromParcel(const android::hardware::Parcel& parcel);

// ---------------------- Status

// Bear in mind that if the client or service is a Java endpoint, this
// is not the logic which will provide/interpret the data here.
status_t readFromParcel(Status *status, const Parcel& parcel);
status_t writeToParcel(const Status &status, Parcel* parcel);

// ---------------------- hidl_vec

template<typename T>
status_t readEmbeddedFromParcel(
        hidl_vec<T> * /*vec*/,
        const Parcel &parcel,
        size_t parentHandle,
        size_t parentOffset,
        size_t *handle) {
    const void *ptr = parcel.readEmbeddedBuffer(
            handle,
            parentHandle,
            parentOffset + hidl_vec<T>::kOffsetOfBuffer);

    return ptr != NULL ? OK : UNKNOWN_ERROR;
}

template<typename T>
status_t writeEmbeddedToParcel(
        const hidl_vec<T> &vec,
        Parcel *parcel,
        size_t parentHandle,
        size_t parentOffset,
        size_t *handle) {
    return parcel->writeEmbeddedBuffer(
            vec.data(),
            sizeof(T) * vec.size(),
            handle,
            parentHandle,
            parentOffset + hidl_vec<T>::kOffsetOfBuffer);
}

template<typename T>
status_t findInParcel(const hidl_vec<T> &vec, const Parcel &parcel, size_t *handle) {
    return parcel.quickFindBuffer(vec.data(), handle);
}

// ---------------------- MQDescriptor

template<MQFlavor flavor>
::android::status_t readEmbeddedFromParcel(
        MQDescriptor<flavor> *obj,
        const ::android::hardware::Parcel &parcel,
        size_t parentHandle,
        size_t parentOffset) {
    ::android::status_t _hidl_err = ::android::OK;

    size_t _hidl_grantors_child;

    _hidl_err = ::android::hardware::readEmbeddedFromParcel(
                &obj->grantors(),
                parcel,
                parentHandle,
                parentOffset + MQDescriptor<flavor>::kOffsetOfGrantors,
                &_hidl_grantors_child);

    if (_hidl_err != ::android::OK) { return _hidl_err; }

    const native_handle_t *_hidl_mq_handle_ptr = parcel.readEmbeddedNativeHandle(
            parentHandle,
            parentOffset + MQDescriptor<flavor>::kOffsetOfHandle);

    if (_hidl_mq_handle_ptr == nullptr) {
        _hidl_err = ::android::UNKNOWN_ERROR;
        return _hidl_err;
    }

    return _hidl_err;
}

template<MQFlavor flavor>
::android::status_t writeEmbeddedToParcel(
        const MQDescriptor<flavor> &obj,
        ::android::hardware::Parcel *parcel,
        size_t parentHandle,
        size_t parentOffset) {
    ::android::status_t _hidl_err = ::android::OK;

    size_t _hidl_grantors_child;

    _hidl_err = ::android::hardware::writeEmbeddedToParcel(
            obj.grantors(),
            parcel,
            parentHandle,
            parentOffset + MQDescriptor<flavor>::kOffsetOfGrantors,
            &_hidl_grantors_child);

    if (_hidl_err != ::android::OK) { return _hidl_err; }

    _hidl_err = parcel->writeEmbeddedNativeHandle(
            obj.handle(),
            parentHandle,
            parentOffset + MQDescriptor<flavor>::kOffsetOfHandle);

    if (_hidl_err != ::android::OK) { return _hidl_err; }

    return _hidl_err;
}

// ---------------------- pointers for HIDL

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

// ---------------------- support for casting interfaces

// Construct a smallest possible binder from the given interface.
// If it is remote, then its remote() will be retrieved.
// Otherwise, the smallest possible BnChild is found where IChild is a subclass of IType
// and iface is of class IChild. BnChild will be used to wrapped the given iface.
// Return nullptr if iface is null or any failure.
template <typename IType, typename IHwType>
sp<IBinder> toBinder(sp<IType> iface) {
    IType *ifacePtr = iface.get();
    if (ifacePtr == nullptr) {
        return nullptr;
    }
    if (ifacePtr->isRemote()) {
        return ::android::hardware::IInterface::asBinder(static_cast<IHwType *>(ifacePtr));
    } else {
        std::string myDescriptor{};
        ifacePtr->interfaceChain([&](const hidl_vec<hidl_string> &types) {
            if (types.size() > 0) {
                myDescriptor = types[0].c_str();
            }
        });
        if (myDescriptor.empty()) {
            // interfaceChain fails
            return nullptr;
        }
        auto iter = gBnConstructorMap.find(myDescriptor);
        if (iter == gBnConstructorMap.end()) {
            return nullptr;
        }
        return sp<IBinder>((iter->second)(reinterpret_cast<void *>(ifacePtr)));
    }
}

#define IMPLEMENT_SERVICE_MANAGER_INTERACTIONS(INTERFACE, PACKAGE)                       \
    ::android::sp<I##INTERFACE> I##INTERFACE::getService(                                \
            const std::string &serviceName, bool getStub)                                \
    {                                                                                    \
        using ::android::sp;                                                             \
        using ::android::hardware::defaultServiceManager;                                \
        using ::android::hidl::manager::V1_0::IServiceManager;                           \
        sp<I##INTERFACE> iface;                                                          \
        const sp<IServiceManager> sm = defaultServiceManager();                          \
        if (sm != nullptr && !getStub) {                                                 \
            sp<::android::hidl::base::V1_0::IBase> base;                                 \
            ::android::hardware::Return<void> ret =                                      \
                sm->get(PACKAGE "::I" #INTERFACE, serviceName.c_str(),                   \
                    [&base](sp<::android::hidl::base::V1_0::IBase> found) {              \
                        base = found;                                                    \
                    });                                                                  \
            if (ret.getStatus().isOk()) {                                                \
                iface = I##INTERFACE::castFrom(base);                                    \
                if (iface != nullptr) {                                                  \
                    return iface;                                                        \
                }                                                                        \
            }                                                                            \
        }                                                                                \
        int dlMode = RTLD_LAZY;                                                          \
        void *handle = dlopen(HAL_LIBRARY_PATH_ODM PACKAGE "-impl.so", dlMode);          \
        if (handle == nullptr) {                                                         \
            handle = dlopen(HAL_LIBRARY_PATH_VENDOR PACKAGE "-impl.so", dlMode);         \
        }                                                                                \
        if (handle == nullptr) {                                                         \
            handle = dlopen(HAL_LIBRARY_PATH_SYSTEM PACKAGE "-impl.so", dlMode);         \
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
    ::android::status_t I##INTERFACE::registerAsService(                                 \
            const std::string &serviceName)                                              \
    {                                                                                    \
        using ::android::sp;                                                             \
        using ::android::hardware::defaultServiceManager;                                \
        using ::android::hidl::manager::V1_0::IServiceManager;                           \
        sp<Bn##INTERFACE> binderIface = new Bn##INTERFACE(this);                         \
        const sp<IServiceManager> sm = defaultServiceManager();                          \
        bool success = false;                                                            \
        ::android::hardware::Return<void> ret =                                          \
            this->interfaceChain(                                                        \
                [&success, &sm, &serviceName, &binderIface](const auto &chain) {         \
                    ::android::hardware::Return<bool> addRet =                           \
                            sm->add(chain, serviceName.c_str(), binderIface);            \
                    success = addRet.isOk() && addRet;                                   \
                });                                                                      \
        success = success && ret.getStatus().isOk();                                     \
        return success ? ::android::OK : ::android::UNKNOWN_ERROR;                       \
    }                                                                                    \
    bool I##INTERFACE::registerForNotifications(                                         \
            const std::string &serviceName,                                              \
            const ::android::sp<::android::hidl::manager::V1_0::IServiceNotification>    \
                      &notification)                                                     \
    {                                                                                    \
        using ::android::sp;                                                             \
        using ::android::hardware::defaultServiceManager;                                \
        using ::android::hidl::manager::V1_0::IServiceManager;                           \
        const sp<IServiceManager> sm = defaultServiceManager();                          \
        if (sm == nullptr) {                                                             \
            return false;                                                                \
        }                                                                                \
        ::android::hardware::Return<bool> success =                                      \
                sm->registerForNotifications(PACKAGE "::I" #INTERFACE,                   \
                                             serviceName,                                \
                                             notification);                              \
        return success.isOk() && success;                                                \
    }


}  // namespace hardware
}  // namespace android


#endif  // ANDROID_HIDL_BINDER_SUPPORT_H
