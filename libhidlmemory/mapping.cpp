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
#define LOG_TAG "libhidlmemory"

#include <hidlmemory/mapping.h>

#include <android-base/logging.h>
#include <android/hidl/memory/1.0/IMapper.h>
#include <hidl/HidlSupport.h>

using android::sp;
using android::hidl::memory::V1_0::IMemory;

namespace android {
namespace hardware {

sp<IMemory> mapMemory(const hidl_memory &memory) {
    using android::hidl::memory::V1_0::IMapper;

    sp<IMapper> mapper = IMapper::getService(memory.name(), true /* getStub */);

    if (mapper == nullptr) {
        LOG(FATAL) << "Could not fetch mapper for " << memory.name() << " shared memory";
    }

    if (mapper->isRemote()) {
        LOG(FATAL) << "IMapper must be a passthrough service.";
    }

    sp<IMemory> retMemory = nullptr;

    Return<void> ret = mapper->mapMemory(memory,
        [&retMemory](const auto &mapped) {
            retMemory = mapped;
        });

    if (!ret.isOk()) {
        LOG(FATAL) << "hidl_memory map returned transport error.";
    }

    return retMemory;
}

}  // namespace hardware
}  // namespace android