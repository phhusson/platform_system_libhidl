/*
 * Copyright (C) 2017 The Android Open Source Project
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
#include <hidl/HidlTransportSupport.h>
#include <hidl/HidlBinderSupport.h>

namespace android {
namespace hardware {

void configureRpcThreadpool(size_t maxThreads, bool callerWillJoin) {
    // TODO(b/32756130) this should be transport-dependent
    configureBinderRpcThreadpool(maxThreads, callerWillJoin);
}
void joinRpcThreadpool() {
    // TODO(b/32756130) this should be transport-dependent
    joinBinderRpcThreadpool();
}

// TODO(b/122472540): only store one data item per object
template <typename V>
static void pruneMapLocked(ConcurrentMap<wp<::android::hidl::base::V1_0::IBase>, V>& map) {
    using ::android::hidl::base::V1_0::IBase;

    std::vector<wp<IBase>> toDelete;
    for (const auto& kv : map) {
        if (kv.first.promote() == nullptr) {
            toDelete.push_back(kv.first);
        }
    }
    for (const auto& k : toDelete) {
        map.eraseLocked(k);
    }
}

bool setMinSchedulerPolicy(const sp<::android::hidl::base::V1_0::IBase>& service,
                           int policy, int priority) {
    if (service->isRemote()) {
        ALOGE("Can't set scheduler policy on remote service.");
        return false;
    }

    if (policy != SCHED_NORMAL && policy != SCHED_FIFO && policy != SCHED_RR) {
        ALOGE("Invalid scheduler policy %d", policy);
        return false;
    }

    if (policy == SCHED_NORMAL && (priority < -20 || priority > 19)) {
        ALOGE("Invalid priority for SCHED_NORMAL: %d", priority);
        return false;
    } else if (priority < 1 || priority > 99) {
        ALOGE("Invalid priority for real-time policy: %d", priority);
        return false;
    }

    details::gServicePrioMap.set(service, { policy, priority });

    return true;
}

bool setRequestingSid(const sp<::android::hidl::base::V1_0::IBase>& service, bool requesting) {
    if (service->isRemote()) {
        ALOGE("Can't set requesting sid on remote service.");
        return false;
    }

    // Due to ABI considerations, IBase cannot have a destructor to clean this up.
    // So, because this API is so infrequently used, (expected to be usually only
    // one time for a process, but it can be more), we are cleaning it up here.
    std::unique_lock<std::mutex> lock = details::gServiceSidMap.lock();
    pruneMapLocked(details::gServiceSidMap);
    details::gServiceSidMap.setLocked(service, requesting);

    return true;
}

}
}
