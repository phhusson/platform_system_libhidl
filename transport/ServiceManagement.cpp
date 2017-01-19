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

#include <android-base/logging.h>
#include <dlfcn.h>
#include <hidl-util/FQName.h>
#include <hwbinder/IPCThreadState.h>
#include <hwbinder/Parcel.h>
#include <unistd.h>

#include <android/hidl/manager/1.0/IServiceManager.h>
#include <android/hidl/manager/1.0/BpHwServiceManager.h>
#include <android/hidl/manager/1.0/BnHwServiceManager.h>

using android::hidl::manager::V1_0::IServiceManager;
using android::hidl::manager::V1_0::IServiceNotification;
using android::hidl::manager::V1_0::BpHwServiceManager;
using android::hidl::manager::V1_0::BnHwServiceManager;

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
            gDefaultServiceManager = fromBinder<IServiceManager, BpHwServiceManager, BnHwServiceManager>(
                ProcessState::self()->getContextObject(NULL));
            if (gDefaultServiceManager == NULL)
                sleep(1);
        }
    }

    return gDefaultServiceManager;
}

struct PassthroughServiceManager : IServiceManager {
    Return<sp<IBase>> get(const hidl_string& fqName,
                     const hidl_string& name) override {
        FQName iface(fqName);

        if (!iface.isValid() ||
            !iface.isFullyQualified() ||
            iface.isIdentifier()) {
            LOG(ERROR) << "Invalid interface name passthrough lookup: " << fqName;
            return nullptr;
        }

        const int dlMode = RTLD_LAZY;
        void *handle = nullptr;

        for (const std::string &path : {
            HAL_LIBRARY_PATH_ODM, HAL_LIBRARY_PATH_VENDOR, HAL_LIBRARY_PATH_SYSTEM
        }) {
            const std::string lib = path + iface.getPackageAndVersion().string() + "-impl.so";
            handle = dlopen(lib.c_str(), dlMode);
            if (handle != nullptr) {
                break;
            }
        }

        if (handle == nullptr) {
            return nullptr;
        }

        const std::string sym = "HIDL_FETCH_" + iface.name();

        IBase* (*generator)(const char* name);
        *(void **)(&generator) = dlsym(handle, sym.c_str());
        if(!generator) {
            return nullptr;
        }
        return (*generator)(name);
    }

    Return<bool> add(const hidl_vec<hidl_string>& /* interfaceChain */,
                     const hidl_string& /* name */,
                     const sp<IBase>& /* service */) override {
        LOG(FATAL) << "Cannot register services with passthrough service manager.";
        return false;
    }

    Return<void> list(list_cb /* _hidl_cb */) override {
        // TODO: add this functionality
        LOG(FATAL) << "Cannot list services with passthrough service manager.";
        return Void();
    }
    Return<void> listByInterface(const hidl_string& /* fqInstanceName */,
                                 listByInterface_cb /* _hidl_cb */) override {
        // TODO: add this functionality
        LOG(FATAL) << "Cannot list services with passthrough service manager.";
        return Void();
    }

    Return<bool> registerForNotifications(const hidl_string& /* fqName */,
                                          const hidl_string& /* name */,
                                          const sp<IServiceNotification>& /* callback */) override {
        // This makes no sense.
        LOG(FATAL) << "Cannot register for notifications with passthrough service manager.";
        return false;
    }

};

sp<IServiceManager> getPassthroughServiceManager() {
    static sp<PassthroughServiceManager> manager(new PassthroughServiceManager());
    return manager;
}

}; // namespace hardware
}; // namespace android
