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
#define LOG_TAG "libhidltransport"

#include <hidl/LegacySupport.h>

#include <condition_variable>
#include <cutils/properties.h>
#include <mutex>
#include <utils/misc.h>
#include <utils/Log.h>

namespace android {
namespace hardware {

static const char* kDataProperty = "vold.post_fs_data_done";
static std::mutex gDataMutex;
static std::condition_variable gDataCondition;
static bool gDataDone = false;
static bool gDataStarted = false;

static void voldDecryptCallback() {
    std::lock_guard<std::mutex> lock(gDataMutex);

    if (gDataDone) {
        return; // TODO: add remove_sysprop_change_callback
    }

    // more expensive to query property_get_bool than to check the lock
    if (!property_get_bool(kDataProperty, false)) {
        return; // other sysprop set, and this hasn't been set yet
    }

    // file system is mounted!

    gDataDone = true;
    gDataCondition.notify_all();
}

void waitForData() {
    {
        std::unique_lock<std::mutex> lock(gDataMutex);

        // unlikely, but we should make sure two threads in the same process
        // don't call this method at the same time
        if (gDataStarted) {
            // just wait to be notified
            gDataCondition.wait(lock, [] () {
                return gDataDone;
            });
            return;
        }
    }

    add_sysprop_change_callback(voldDecryptCallback, 0 /* priority */);

    // if it already got setup before we registered the decrypt callback,
    // we must still wake up.
    voldDecryptCallback();

    std::unique_lock<std::mutex> lock(gDataMutex);
    gDataCondition.wait(lock, [] () {
        return gDataDone;
    });

    gDataStarted = true;
}

namespace details {

bool blockingHalBinderizationEnabled() {
    waitForData();
    return property_get_bool("persist.hal.binderization", false);
}

void blockIfBinderizationDisabled(const std::string& interface,
                                  const std::string& instance) {
    // TODO(b/34274385) remove this
    // Must wait for data to be mounted and persistant properties to be read,
    // but only delay the start of hals which require reading this property.
    bool enabled = blockingHalBinderizationEnabled();

    if (!enabled) {
        ALOGI("Deactivating %s/%s binderized service to"
              " yield to passthrough implementation.",
              interface.c_str(),
              instance.c_str());
        while (true) {
            sleep(UINT_MAX);
        }
    }
}
}  // namespace details

}  // namespace hardware
}  // namespace android
