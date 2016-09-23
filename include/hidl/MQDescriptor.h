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

struct GrantorDescriptor {
    uint32_t flags;
    uint32_t fdIndex;
    uint32_t offset;
    uint32_t extent;
};

enum GrantorType : int { READPTRPOS = 0, WRITEPTRPOS, DATAPTRPOS };

struct MQDescriptor {
    MQDescriptor(
            const std::vector<GrantorDescriptor>& grantors,
            native_handle_t* nhandle, int32_t flags, size_t size);

    ~MQDescriptor();

    explicit MQDescriptor(const MQDescriptor &other);
    MQDescriptor &operator=(const MQDescriptor &other) = delete;

    size_t getSize() const;

    size_t getQuantum() const;

    int32_t getFlags() const;

    ::android::status_t readEmbeddedFromParcel(
            const ::android::hardware::Parcel &parcel,
            size_t parentHandle,
            size_t parentOffset);

    ::android::status_t writeEmbeddedToParcel(
            ::android::hardware::Parcel *parcel,
            size_t parentHandle,
            size_t parentOffset) const;

    bool isHandleValid() const { return mHandle != nullptr; }
    size_t countGrantors() const { return mGrantors.size(); }
    std::vector<GrantorDescriptor> getGrantors() const;
    const sp<NativeHandle> getNativeHandle() const;

private:
    ::android::hardware::hidl_vec<GrantorDescriptor> mGrantors;
    ::native_handle_t *mHandle;
    uint32_t mQuantum;
    uint32_t mFlags;
};
}  // namespace hardware
}  // namespace android

#endif  // FMSGQ_DESCRIPTOR_H
