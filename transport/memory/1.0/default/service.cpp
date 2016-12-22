#define LOG_TAG "android.hidl.memory@1.0-service"

#include "AshmemAllocator.h"

#include <android-base/logging.h>
#include <android/hidl/memory/1.0/IAllocator.h>
#include <hwbinder/IPCThreadState.h>
#include <hwbinder/ProcessState.h>

using android::hardware::IPCThreadState;
using android::hardware::ProcessState;
using android::hidl::memory::V1_0::IAllocator;
using android::hidl::memory::V1_0::implementation::AshmemAllocator;
using android::sp;
using android::status_t;

int main() {
    sp<IAllocator> allocator = new AshmemAllocator();

    status_t status = allocator->registerAsService("ashmem");

    if (android::OK != status) {
        LOG(FATAL) << "Unable to register allocator service: " << status;
    }

    ProcessState::self()->setThreadPoolMaxThreadCount(0);
    ProcessState::self()->startThreadPool();
    IPCThreadState::self()->joinThreadPool();

    return -1;
}
