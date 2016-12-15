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

#define LOG_TAG "ServiceManagement"

#include <hidl/HidlBinderSupport.h>
#include <hidl/ServiceManagement.h>
#include <hidl/Static.h>
#include <hidl/Status.h>

#include <hwbinder/IPCThreadState.h>
#include <hwbinder/Parcel.h>
#include <utils/Log.h>
#include <utils/SystemClock.h>

#include <unistd.h>

#include <android/hidl/manager/1.0/IServiceManager.h>
#include <android/hidl/manager/1.0/BpServiceManager.h>
#include <android/hidl/manager/1.0/BnServiceManager.h>

using android::hidl::manager::V1_0::IServiceManager;
using android::hidl::manager::V1_0::BpServiceManager;
using android::hidl::manager::V1_0::BnServiceManager;

namespace android {
namespace hardware {

sp<IServiceManager> defaultServiceManager() {

    if (gDefaultServiceManager != NULL) return gDefaultServiceManager;
    if (access("/dev/hwbinder", F_OK|R_OK|W_OK) != 0) {
        // HwBinder not available on this device or not accessible to
        // this process.
        return nullptr;
    }
    {
        AutoMutex _l(gDefaultServiceManagerLock);
        while (gDefaultServiceManager == NULL) {
            gDefaultServiceManager = fromBinder<IServiceManager, BpServiceManager, BnServiceManager>(
                ProcessState::self()->getContextObject(NULL));
            if (gDefaultServiceManager == NULL)
                sleep(1);
        }
    }

    return gDefaultServiceManager;
}

}; // namespace hardware
}; // namespace android
