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

#ifndef ANDROID_HIDL_TRANSPORT_SUPPORT_H
#define ANDROID_HIDL_TRANSPORT_SUPPORT_H

#include <hidl/HidlSupport.h>
#include <hidl/HidlBinderSupport.h>

namespace android {
namespace hardware {

// cast the interface IParent to IChild.
// Return nullptr if parent is null or any failure.
template<typename IChild, typename IParent, typename BpChild, typename IHwParent>
sp<IChild> castInterface(sp<IParent> parent, const char *childIndicator) {
    if (parent.get() == nullptr) {
        // casts always succeed with nullptrs.
        return nullptr;
    }
    bool canCast = false;
    parent->interfaceChain([&](const hidl_vec<hidl_string> &allowedCastTypes) {
        for (size_t i = 0; i < allowedCastTypes.size(); i++) {
            if (allowedCastTypes[i] == childIndicator) {
                canCast = true;
                break;
            }
        }
    });

    if (!canCast) {
        return sp<IChild>(nullptr); // cast failed.
    }
    // TODO b/32001926 Needs to be fixed for socket mode.
    if (parent->isRemote()) {
        // binderized mode. Got BpChild. grab the remote and wrap it.
        return sp<IChild>(new BpChild(toBinder<IParent, IHwParent>(parent)));
    }
    // Passthrough mode. Got BnChild and BsChild.
    return sp<IChild>(static_cast<IChild *>(parent.get()));
}

}  // namespace hardware
}  // namespace android


#endif  // ANDROID_HIDL_TRANSPORT_SUPPORT_H
