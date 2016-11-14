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

#include <utils/Log.h>

#include <hwbinder/IPCThreadState.h>
#include <hwbinder/ProcessState.h>
#include <utils/Errors.h>
#include <utils/StrongPointer.h>

#ifndef ANDROID_HIDL_LEGACY_SUPPORT_H
#define ANDROID_HIDL_LEGACY_SUPPORT_H

namespace android {
namespace hardware {

/**
 * Registers passthrough service implementation.
 */
template<class Interface>
status_t registerPassthroughServiceImplementation(std::string name) {
    sp<Interface> service = Interface::getService(name, true /* getStub */);

    if (service == nullptr) {
        ALOGE("Could not get passthrough implementation for %s.", name.c_str());
        return EXIT_FAILURE;
    }

    LOG_FATAL_IF(service->isRemote(), "Implementation of %s is remote!", name.c_str());

    status_t status = service->registerAsService(name);

    if (status == OK) {
        ALOGI("Registration complete for %s.", name.c_str());
    } else {
        ALOGE("Could not register service %s (%d).", name.c_str(), status);
    }

    return status;
}

/**
 * Launches the RPC threadpool. This method never returns.
 *
 * Return value is exit status.
 */
inline int launchRpcServer(size_t maxThreads) {
    ProcessState::self()->setThreadPoolMaxThreadCount(maxThreads);
    ProcessState::self()->startThreadPool();
    IPCThreadState::self()->joinThreadPool();

    return 0;
}

/**
 * Creates default passthrough service implementation. This method never returns.
 *
 * Return value is exit status.
 */
template<class Interface>
int defaultPassthroughServiceImplementation(std::string name) {
    registerPassthroughServiceImplementation<Interface>(name);
    return launchRpcServer(0);
}

}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HIDL_LEGACY_SUPPORT_H
