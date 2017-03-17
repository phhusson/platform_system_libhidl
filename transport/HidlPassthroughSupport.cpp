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
#include <hidl/HidlPassthroughSupport.h>

#include <hidl/HidlSupport.h>
#include <hidl/HidlTransportUtils.h>
#include <hidl/Static.h>

namespace android {
namespace hardware {
namespace details {

sp<::android::hidl::base::V1_0::IBase> wrapPassthrough(
        sp<::android::hidl::base::V1_0::IBase> iface) {
    if (iface.get() == nullptr || iface->isRemote()) {
        // doesn't know how to handle it.
        return iface;
    }
    std::string myDescriptor = getDescriptor(iface.get());
    if (myDescriptor.empty()) {
        // interfaceDescriptor fails
        return nullptr;
    }
    auto func = gBsConstructorMap.get(myDescriptor, nullptr);
    if (!func) {
        return nullptr;
    }
    return func(reinterpret_cast<void *>(iface.get()));
}

}  // namespace details
}  // namespace hardware
}  // namespace android
