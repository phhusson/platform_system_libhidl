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

#include "AshmemAllocator.h"

#include <cutils/ashmem.h>

namespace android {
namespace hidl {
namespace memory {
namespace V1_0 {
namespace implementation {

// Methods from ::android::hidl::memory::V1_0::IAllocator follow.
Return<void> AshmemAllocator::allocate(uint64_t size, allocate_cb _hidl_cb) {
    int fd = ashmem_create_region("AshmemAllocator_hidl", size);
    if (fd < 0) {
        _hidl_cb(false /* success */, hidl_memory());
        return Void();
    }

    native_handle_t* handle = native_handle_create(1, 0);
    handle->data[0] = fd;
    hidl_memory memory("ashmem", handle, size);

    _hidl_cb(true /* success */, memory);
    return Void();
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace memory
}  // namespace hidl
}  // namespace android
