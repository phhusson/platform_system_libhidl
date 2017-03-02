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

#include <inttypes.h>

#include <hidl/LegacySupport.h>

#include <chrono>
#include <thread>
#include <utils/misc.h>
#include <utils/Log.h>
#include <utils/SystemClock.h>

#include <android-base/properties.h>

namespace android {
namespace hardware {

namespace details {

using android::base::GetBoolProperty;
using android::base::WaitForPropertyCreation;

static const char* kPersistPropReadyProperty = "ro.persistent_properties.ready";
static const char* kBinderizationProperty = "persist.hal.binderization";

bool blockingHalBinderizationEnabled() {
    if (!GetBoolProperty(kPersistPropReadyProperty, false)) { // not set yet
        int64_t startTime = elapsedRealtime();
        WaitForPropertyCreation(kPersistPropReadyProperty, std::chrono::milliseconds::max());
        ALOGI("Waiting for %s took %" PRId64 " ms", kPersistPropReadyProperty,
              elapsedRealtime() - startTime);
    }
    return GetBoolProperty(kBinderizationProperty, false);
}

void blockIfBinderizationDisabled(const std::string& interface,
                                  const std::string& instance) {
    // TODO(b/34274385) remove this

    size_t loc = interface.find_first_of("@");
    if (loc == std::string::npos) {
        LOG_ALWAYS_FATAL("Bad interface name: %s", interface.c_str());
    }
    std::string package = interface.substr(0, loc);

    // only block if this is supposed to be toggled
    if (getTransport(interface, instance) != vintf::Transport::TOGGLED) {
        return;
    }

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
