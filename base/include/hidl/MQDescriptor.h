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

#ifndef _FMSGQ_DESCRIPTOR_H
#define _FMSGQ_DESCRIPTOR_H

#include <android-base/macros.h>
#include <cutils/native_handle.h>
#include <hidl/HidlSupport.h>
#include <utils/NativeHandle.h>

namespace android {
namespace hardware {

typedef uint64_t RingBufferPosition;

struct GrantorDescriptor {
    uint32_t flags;
    uint32_t fdIndex;
    uint32_t offset;
    size_t extent;
};

enum MQFlavor : uint32_t {
  /*
   * kSynchronizedReadWrite represents the wait-free synchronized flavor of the
   * FMQ. It is intended to be have a single reader and single writer.
   * Attempts to overflow/underflow returns a failure.
   */
  kSynchronizedReadWrite = 0x01,
  /*
   * kUnsynchronizedWrite represents the flavor of FMQ where writes always
   * succeed. This flavor allows one writer and many readers. A read operation
   * can detect an overwrite and reset the read counter.
   */
  kUnsynchronizedWrite = 0x02
};

template <MQFlavor flavor>
struct MQDescriptor {
    MQDescriptor(
            const std::vector<GrantorDescriptor>& grantors,
            native_handle_t* nHandle, size_t size);

    MQDescriptor(size_t bufferSize, native_handle_t* nHandle,
                 size_t messageSize);

    ~MQDescriptor();

    explicit MQDescriptor(const MQDescriptor &other);
    MQDescriptor &operator=(const MQDescriptor &other) = delete;

    size_t getSize() const;

    size_t getQuantum() const;

    int32_t getFlags() const;

    bool isHandleValid() const { return mHandle != nullptr; }
    size_t countGrantors() const { return mGrantors.size(); }
    std::vector<GrantorDescriptor> getGrantors() const;
    const sp<NativeHandle> getNativeHandle() const;

    inline const ::android::hardware::hidl_vec<GrantorDescriptor> &grantors() const {
        return mGrantors;
    }

    inline ::android::hardware::hidl_vec<GrantorDescriptor> &grantors() {
        return mGrantors;
    }

    inline const ::native_handle_t *handle() const {
        return mHandle;
    }

    inline ::native_handle_t *handle() {
        return mHandle;
    }

    static const size_t kOffsetOfGrantors;
    static const size_t kOffsetOfHandle;
    /*
     * There should atleast be GrantorDescriptors for the read counter, write
     * counter and data buffer.
     */
    static constexpr int32_t kMinGrantorCount = 3;
    enum GrantorType : int { READPTRPOS = 0, WRITEPTRPOS, DATAPTRPOS };
private:
    ::android::hardware::hidl_vec<GrantorDescriptor> mGrantors;
    ::android::hardware::details::hidl_pointer<native_handle_t> mHandle;
    uint32_t mQuantum;
    uint32_t mFlags;
};

template<MQFlavor flavor>
const size_t MQDescriptor<flavor>::kOffsetOfGrantors = offsetof(MQDescriptor<flavor>, mGrantors);

template<MQFlavor flavor>
const size_t MQDescriptor<flavor>::kOffsetOfHandle = offsetof(MQDescriptor<flavor>, mHandle);

/*
 * MQDescriptorSync will describe the wait-free synchronized
 * flavor of FMQ.
 */
using MQDescriptorSync = MQDescriptor<kSynchronizedReadWrite>;

/*
 * MQDescriptorUnsync will describe the unsynchronized write
 * flavor of FMQ.
 */
using MQDescriptorUnsync = MQDescriptor<kUnsynchronizedWrite>;

template<MQFlavor flavor>
MQDescriptor<flavor>::MQDescriptor(
        const std::vector<GrantorDescriptor>& grantors,
        native_handle_t* nhandle,
        size_t size)
    : mHandle(nhandle),
      mQuantum(size),
      mFlags(flavor) {
    mGrantors.resize(grantors.size());
    for (size_t i = 0; i < grantors.size(); ++i) {
        mGrantors[i] = grantors[i];
    }
}

template<MQFlavor flavor>
MQDescriptor<flavor>::MQDescriptor(size_t bufferSize, native_handle_t *nHandle,
                           size_t messageSize)
    : mHandle(nHandle), mQuantum(messageSize), mFlags(flavor) {
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

template<MQFlavor flavor>
MQDescriptor<flavor>::MQDescriptor(const MQDescriptor<flavor> &other)
    : mGrantors(other.mGrantors),
      mHandle(nullptr),
      mQuantum(other.mQuantum),
      mFlags(other.mFlags) {
    if (other.mHandle != nullptr) {
        mHandle = native_handle_create(
                other.mHandle->numFds, other.mHandle->numInts);

        for (int i = 0; i < other.mHandle->numFds; ++i) {
            mHandle->data[i] = dup(other.mHandle->data[i]);
        }

        memcpy(&mHandle->data[other.mHandle->numFds],
               &other.mHandle->data[other.mHandle->numFds],
               other.mHandle->numInts * sizeof(int));
    }
}

template<MQFlavor flavor>
MQDescriptor<flavor>::~MQDescriptor() {
    if (mHandle != nullptr) {
        native_handle_close(mHandle);
        native_handle_delete(mHandle);
    }
}

template<MQFlavor flavor>
size_t MQDescriptor<flavor>::getSize() const {
  return mGrantors[DATAPTRPOS].extent;
}

template<MQFlavor flavor>
size_t MQDescriptor<flavor>::getQuantum() const { return mQuantum; }

template<MQFlavor flavor>
int32_t MQDescriptor<flavor>::getFlags() const { return mFlags; }

template<MQFlavor flavor>
std::vector<GrantorDescriptor> MQDescriptor<flavor>::getGrantors() const {
  size_t grantor_count = mGrantors.size();
  std::vector<GrantorDescriptor> grantors(grantor_count);
  for (size_t i = 0; i < grantor_count; i++) {
    grantors[i] = mGrantors[i];
  }
  return grantors;
}

template<MQFlavor flavor>
const sp<NativeHandle> MQDescriptor<flavor>::getNativeHandle() const {
  /*
   * Create an sp<NativeHandle> from mHandle.
   */
  return NativeHandle::create(mHandle, false /* ownsHandle */);
}
}  // namespace hardware
}  // namespace android

#endif  // FMSGQ_DESCRIPTOR_H
