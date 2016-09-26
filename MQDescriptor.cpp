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

#define LOG_TAG "MQDescriptor"
#include <android-base/logging.h>

#include <hidl/MQDescriptor.h>

#include <cutils/native_handle.h>
#include <sys/mman.h>

namespace android {
namespace hardware {

MQDescriptor::MQDescriptor(
        const std::vector<GrantorDescriptor>& grantors,
        native_handle_t* nhandle,
        int32_t flags,
        size_t size)
    : mHandle(nhandle),
      mQuantum(size),
      mFlags(flags) {
    mGrantors.resize(grantors.size());
    for (size_t i = 0; i < grantors.size(); ++i) {
        mGrantors[i] = grantors[i];
    }
}

MQDescriptor::MQDescriptor(size_t bufferSize, native_handle_t *nHandle,
                           int32_t flags, size_t messageSize)
    : mHandle(nHandle), mQuantum(messageSize), mFlags(flags) {
    mGrantors.resize(kMinGrantorCount);
    /*
     * Create a default grantor descriptor for read, write pointers and
     * the data buffer. fdIndex parameter is set to 0 by default and
     * the offset for each grantor is contiguous.
     */
    mGrantors[READPTRPOS] = {
        0 /* grantor flags */, 0 /* fdIndex */, 0 /* offset */,
        sizeof(RingBufferPosition) /* extent */
    };

    mGrantors[WRITEPTRPOS] = {
        0 /* grantor flags */,
        0 /* fdIndex */,
        sizeof(RingBufferPosition) /* offset */,
        sizeof(RingBufferPosition) /* extent */
    };
    mGrantors[DATAPTRPOS] = {
        0 /* grantor flags */, 0 /* fdIndex */,
        2 * sizeof(RingBufferPosition) /* offset */, bufferSize /* extent */
    };
}

MQDescriptor::MQDescriptor(const MQDescriptor &other)
    : mGrantors(other.mGrantors),
      mHandle(nullptr),
      mQuantum(other.mQuantum),
      mFlags(other.mFlags) {
    if (other.mHandle != nullptr) {
        mHandle = native_handle_create(
                other.mHandle->numFds, other.mHandle->numInts);

        for (int i = 0; i < other.mHandle->numFds; ++i) {
            const_cast<native_handle_t *>(mHandle)->data[i] =
                dup(other.mHandle->data[i]);
        }

        memcpy(&const_cast<native_handle_t *>(mHandle)->data[other.mHandle->numFds],
               &other.mHandle->data[other.mHandle->numFds],
               other.mHandle->numInts * sizeof(int));
    }
}

MQDescriptor::~MQDescriptor() {
    if (mHandle != nullptr) {
        native_handle_close(const_cast<native_handle_t *>(mHandle));
        native_handle_delete(const_cast<native_handle_t *>(mHandle));
    }
}

::android::status_t MQDescriptor::readEmbeddedFromParcel(
    const Parcel &parcel,
    size_t parentHandle,
    size_t parentOffset) {
    ::android::status_t _hidl_err = ::android::OK;

    size_t _hidl_grantors_child;

    _hidl_err = const_cast<hidl_vec<GrantorDescriptor> *>(
            &mGrantors)->readEmbeddedFromParcel(
                parcel,
                parentHandle,
                parentOffset + offsetof(MQDescriptor, mGrantors),
                &_hidl_grantors_child);

    if (_hidl_err != ::android::OK) { return _hidl_err; }

    const native_handle_t *_hidl_mq_handle_ptr = parcel.readEmbeddedNativeHandle(
            parentHandle,
            parentOffset + offsetof(MQDescriptor, mHandle));

    if (_hidl_mq_handle_ptr == nullptr) {
        _hidl_err = ::android::UNKNOWN_ERROR;
        return _hidl_err;
    }

_hidl_error:
    return _hidl_err;
}

::android::status_t MQDescriptor::writeEmbeddedToParcel(
    Parcel *parcel,
    size_t parentHandle,
    size_t parentOffset) const {
    ::android::status_t _hidl_err = ::android::OK;

    size_t _hidl_grantors_child;

    _hidl_err = mGrantors.writeEmbeddedToParcel(
            parcel,
            parentHandle,
            parentOffset + offsetof(MQDescriptor, mGrantors),
            &_hidl_grantors_child);

    if (_hidl_err != ::android::OK) { return _hidl_err; }

    _hidl_err = parcel->writeEmbeddedNativeHandle(
            mHandle,
            parentHandle,
            parentOffset + offsetof(MQDescriptor, mHandle));

    if (_hidl_err != ::android::OK) { return _hidl_err; }

_hidl_error:
    return _hidl_err;
}

size_t MQDescriptor::getSize() const {
  return mGrantors[DATAPTRPOS].extent;
}

size_t MQDescriptor::getQuantum() const { return mQuantum; }

int32_t MQDescriptor::getFlags() const { return mFlags; }

std::vector<GrantorDescriptor> MQDescriptor::getGrantors() const {
  size_t grantor_count = mGrantors.size();
  std::vector<GrantorDescriptor> grantors(grantor_count);
  for (size_t i = 0; i < grantor_count; i++) {
    grantors[i] = mGrantors[i];
  }
  return grantors;
}

const sp<NativeHandle> MQDescriptor::getNativeHandle() const {
  /*
   * Create an sp<NativeHandle> from mHandle.
   */
  return NativeHandle::create(mHandle, false /* ownsHandle */);
}

}  // namespace hardware
}  // namespace android

