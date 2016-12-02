#define LOG_TAG "android.hidl.memory@1.0-service"

#include <android/hidl/memory/1.0/IAllocator.h>

#include <hidl/LegacySupport.h>

using android::hidl::memory::V1_0::IAllocator;
using android::hardware::defaultPassthroughServiceImplementation;

int main() {
    return defaultPassthroughServiceImplementation<IAllocator>("ashmem");
}
